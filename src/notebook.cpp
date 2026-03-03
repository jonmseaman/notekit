#include "notebook.h"
#include "notebook_clipboard.hpp"

#include <QFile>
#include <QTextStream>
#include <QSaveFile>
#include <QFontDatabase>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QDateTime>

// ── parseNoteFile (T010) ──────────────────────────────────────────────────────
// Reads a .md note file and extracts all <drawing data-for="ID">…</drawing> blocks.
// Returns plain text (drawing blocks replaced by empty string) plus ordered drawing data.

NoteData parseNoteFile(const QString &path)
{
    NoteData result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "parseNoteFile: cannot open" << path;
        return result;
    }

    QTextStream ts(&file);
    ts.setEncoding(QStringConverter::Utf8);
    QString content = ts.readAll();
    file.close();

    // Regex to match a full drawing block:
    //   <drawing data-for="ID">\n
    //   ...base64 lines...\n
    //   </drawing>\n?
    static const QRegularExpression drawingRe(
        QStringLiteral("<drawing data-for=\"([^\"]+)\">(.*?)</drawing>"),
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption);

    QString plain;
    plain.reserve(content.size());
    int lastEnd = 0;

    auto it = drawingRe.globalMatch(content);
    while (it.hasNext()) {
        auto match = it.next();

        // Append text before this drawing block
        int textOffset = plain.length() + (match.capturedStart() - lastEnd);
        plain += content.mid(lastEnd, match.capturedStart() - lastEnd);

        DrawingBlock block;
        block.id = match.captured(1);
        // Captured group 2 contains the raw base64 content (with newlines)
        block.base64Data = match.captured(2);

        // Record position in the plain text where this drawing was found
        // Use a single newline as placeholder so text stays readable
        int posInPlain = plain.length();
        result.drawingPositions.append({posInPlain, block});

        lastEnd = match.capturedEnd();
    }

    // Append remaining text
    plain += content.mid(lastEnd);

    result.plainText = plain;
    return result;
}

// ── CNotebook ─────────────────────────────────────────────────────────────────

CNotebook::CNotebook(QWidget *parent)
    : QTextEdit(parent)
{
    setObjectName(QStringLiteral("mainView"));
    setAcceptRichText(false);

    // Set up the markdown syntax highlighter
    m_highlighter = new MarkdownHighlighter(document());

    // Register embedded object handlers
    m_checkboxObj = new CheckboxTextObject(this);
    m_imageObj    = new CImageWidget(this);
    registerEmbeddedObjects(document(), m_checkboxObj, m_imageObj);

    // Load custom font from Qt resources
    setNoteFont();

    // Proximity rendering: rehighlight ±2 blocks on cursor move
    connect(this, &QTextEdit::cursorPositionChanged,
            this, &CNotebook::onCursorPositionChanged);

    // Track modifications for autosave
    connect(document(), &QTextDocument::contentsChanged,
            this, &CNotebook::onContentChanged);

    setPlaceholderText(QStringLiteral("( Nothing opened. Please create or open a file. )"));

    // T027: Create drawing overlay as a child of the viewport
    // The overlay sits on top of the text content and captures events in Draw/Erase mode.
    m_drawingOverlay = new CBoundDrawing(viewport());
    m_drawingOverlay->hide(); // invisible in Text mode
    m_drawingOverlay->resize(viewport()->size());

    // Mark note modified when drawing strokes are added or erased (T027)
    connect(m_drawingOverlay, &CBoundDrawing::strokeAdded, this, [this]() {
        m_drawingModified = true;
        onContentChanged();
    });
    connect(m_drawingOverlay, &CBoundDrawing::strokeErased, this, [this]() {
        m_drawingModified = true;
        onContentChanged();
    });
}

void CNotebook::setNoteFont()
{
    // Load Charter font from Qt resource system
    int id = QFontDatabase::addApplicationFont(
        QStringLiteral(":/notekit/fonts/Charter Regular.otf"));
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/notekit/fonts/Charter Bold.otf"));
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/notekit/fonts/Charter Italic.otf"));
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/notekit/fonts/Charter Bold Italic.otf"));

    QFont font;
    if (id >= 0) {
        QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty())
            font.setFamily(families.first());
    } else {
        font.setFamily(QStringLiteral("Charter"));
    }
    font.setPointSize(12);
    setFont(font);
}

void CNotebook::openNote(const QString &path)
{
    if (path.isEmpty()) {
        m_currentPath.clear();
        m_noteData = {};
        m_loading = true;
        setPlainText(QString());
        setEnabled(false);
        m_loading = false;
        m_lastModified = 0;
        return;
    }

    m_loading = true;
    NoteData data = parseNoteFile(path);
    m_currentPath = path;
    m_noteData    = data;

    rebuildDocument(data);

    // T027: Load the first drawing block into the overlay
    m_drawingId.clear();
    m_drawingModified = false;
    if (m_drawingOverlay) {
        // Clear existing strokes
        m_drawingOverlay->strokes.clear();
        m_drawingOverlay->strokefinder.clear();
        m_drawingOverlay->w = 1;
        m_drawingOverlay->h = 1;
        m_drawingOverlay->Redraw();

        if (!data.drawingPositions.isEmpty()) {
            const auto &[textPos, blk] = data.drawingPositions.first();
            m_drawingId = blk.id;
            m_drawingOverlay->Deserialize(blk.base64Data.toStdString());
        }
    }

    m_loading = false;
    m_lastModified = 0;
    setEnabled(true);
    setReadOnly(false);
}

void CNotebook::rebuildDocument(const NoteData &data)
{
    // Load plain text into the editor (drawing blocks already stripped)
    QTextCursor cur(document());
    cur.beginEditBlock();
    cur.select(QTextCursor::Document);
    cur.removeSelectedText();
    cur.insertText(data.plainText);
    cur.endEditBlock();

    // Move cursor to beginning
    moveCursor(QTextCursor::Start);
}

void CNotebook::saveNote()
{
    if (m_currentPath.isEmpty()) return;

    // T027: Only re-serialize the overlay if the user actually drew/erased something.
    // If unmodified, preserve the original base64Data byte-for-byte (format contract).
    if (m_drawingOverlay && !m_drawingId.isEmpty() && m_drawingModified) {
        std::string serialized = m_drawingOverlay->Serialize();
        for (auto &[pos, blk] : m_noteData.drawingPositions) {
            if (blk.id == m_drawingId) {
                // Serialize() returns "base64line1\nbase64line2\n..."
                // The stored base64Data includes a leading newline (per note format)
                blk.base64Data = QStringLiteral("\n") + QString::fromStdString(serialized);
                break;
            }
        }
    }

    // Reconstruct the file content by re-inserting drawing blocks at their positions.
    // We work in the reverse order (from end to start) so position offsets remain valid.
    QString plain = toPlainText();
    // Qt uses \u2029 (paragraph separator) for line breaks in QTextEdit
    plain.replace(QChar(0x2029), QChar('\n'));

    // Build the output with drawing blocks re-inserted
    // drawingPositions is in forward order; process in reverse to not shift positions
    struct Insertion { int pos; QString block; };
    QList<Insertion> insertions;
    for (const auto &[pos, blk] : m_noteData.drawingPositions) {
        QString blockText = QStringLiteral("<drawing data-for=\"")
                            + blk.id
                            + QStringLiteral("\">")
                            + blk.base64Data
                            + QStringLiteral("</drawing>\n");
        insertions.prepend({pos, blockText}); // reverse order
    }

    QString output = plain;
    for (const auto &ins : insertions) {
        int safePos = qMin(ins.pos, (int)output.length());
        output.insert(safePos, ins.block);
    }

    // Atomic write: write to .tmp then rename
    QString tmpPath = m_currentPath + QStringLiteral(".tmp");
    {
        QSaveFile sf(m_currentPath);
        sf.setDirectWriteFallback(false);
        if (sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&sf);
            ts.setEncoding(QStringConverter::Utf8);
            ts << output;
            if (!sf.commit()) {
                qWarning() << "saveNote: commit failed for" << m_currentPath;
                return;
            }
            document()->setModified(false);
        } else {
            qWarning() << "saveNote: cannot open for writing:" << m_currentPath;
            return;
        }
    }

    m_lastModified = 0; // reset after save
}

bool CNotebook::isModified() const
{
    return document()->isModified();
}

bool CNotebook::findText(const QString &text, bool forward, bool skip)
{
    QTextDocument::FindFlags flags;
    if (!forward)
        flags |= QTextDocument::FindBackward;

    if (skip) {
        QTextCursor cur = textCursor();
        if (forward)
            cur.movePosition(QTextCursor::NextCharacter);
        else
            cur.movePosition(QTextCursor::PreviousCharacter);
        setTextCursor(cur);
    }

    bool found = find(text, flags);
    if (!found) {
        // Wrap around
        QTextCursor cur(document());
        if (forward) cur.movePosition(QTextCursor::Start);
        else         cur.movePosition(QTextCursor::End);
        setTextCursor(cur);
        found = find(text, flags);
    }
    return found;
}

void CNotebook::setProximityEnabled(bool enabled)
{
    if (m_highlighter)
        m_highlighter->setProximityEnabled(enabled);
}

void CNotebook::setInputMode(InputMode mode)
{
    // T028: update overlay visibility and eraseMode flag
    m_inputMode = mode;
    switch (mode) {
    case InputMode::Text:
        if (m_drawingOverlay) {
            m_drawingOverlay->hide();
        }
        viewport()->setCursor(Qt::IBeamCursor);
        setReadOnly(false);
        break;

    case InputMode::Draw:
        if (m_drawingOverlay) {
            m_drawingOverlay->m_eraseMode = false;
            m_drawingOverlay->show();
            m_drawingOverlay->raise();
            m_drawingOverlay->resize(viewport()->size());
        }
        // Use cross cursor for draw mode (pen SVG cursor can be added when icon exists)
        viewport()->setCursor(Qt::CrossCursor);
        break;

    case InputMode::Erase:
        if (m_drawingOverlay) {
            m_drawingOverlay->m_eraseMode = true;
            m_drawingOverlay->show();
            m_drawingOverlay->raise();
            m_drawingOverlay->resize(viewport()->size());
        }
        // Circle-like cursor for erase mode
        viewport()->setCursor(Qt::PointingHandCursor);
        break;

    case InputMode::Select:
        if (m_drawingOverlay) {
            m_drawingOverlay->hide();
        }
        viewport()->setCursor(Qt::ArrowCursor);
        break;
    }
}

void CNotebook::onCursorPositionChanged()
{
    if (m_highlighter) {
        int block = textCursor().blockNumber();
        m_highlighter->proximityRehighlight(block);
    }

    // Check if cursor is over a link
    QTextCursor cur = textCursor();
    QString anchor = cur.charFormat().anchorHref();
    if (!anchor.isEmpty())
        viewport()->setCursor(Qt::PointingHandCursor);
    else if (m_inputMode == InputMode::Text)
        viewport()->setCursor(Qt::IBeamCursor);
}

void CNotebook::onContentChanged()
{
    if (!m_loading) {
        m_lastModified = QDateTime::currentMSecsSinceEpoch();
        emit noteModified();
    }
}

void CNotebook::mousePressEvent(QMouseEvent *event)
{
    // Check for checkbox click
    if (handleCheckboxClick(this, event))
        return;

    // Check for link click (left button)
    if (event->button() == Qt::LeftButton) {
        QString anchor = anchorAt(event->pos());
        if (!anchor.isEmpty()) {
            emit linkActivated(anchor);
            return;
        }
    }

    QTextEdit::mousePressEvent(event);
}

void CNotebook::keyPressEvent(QKeyEvent *event)
{
    // Override cut/copy/paste to use our custom clipboard
    if (event->matches(QKeySequence::Copy)) {
        notebook_copy_clipboard(this);
        return;
    }
    if (event->matches(QKeySequence::Cut)) {
        notebook_cut_clipboard(this);
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        notebook_paste_clipboard(this);
        return;
    }
    QTextEdit::keyPressEvent(event);
}

bool CNotebook::event(QEvent *e)
{
    return QTextEdit::event(e);
}

// T027: Keep drawing overlay sized to match the viewport
void CNotebook::resizeEvent(QResizeEvent *event)
{
    QTextEdit::resizeEvent(event);
    if (m_drawingOverlay)
        m_drawingOverlay->resize(viewport()->size());
}

// T027: Forward active color to drawing overlay (called from mainwindow palette swatches)
void CNotebook::setActiveDrawingColor(const QColor &color)
{
    if (m_drawingOverlay)
        m_drawingOverlay->m_drawColor = color;
}

