#pragma once
// Migrated from Gsv::View (GtkSourceView) to QTextEdit (T012)
// - QSyntaxHighlighter (MarkdownHighlighter) replaces GtkSourceView language specs
// - QTextObjectInterface for embedded checkboxes and drawings
// - proximity rendering via cursorPositionChanged → rehighlightBlock ±2
// T027: CBoundDrawing overlay for Draw/Erase input modes

#include "config.h"

#include <QTextEdit>
#include <QTextDocument>
#include <QMap>
#include <QList>
#include <QPair>
#include <QString>
#include <QByteArray>
#include <QColor>
#include <QTimer>
#include <cstdint>

#include "notebook_highlight.hpp"
#include "notebook_widgets.hpp"
#include "drawing.h"

// Input modes (replaces NB_MODE_* enum)
enum class InputMode { Text, Draw, Erase, Select };

// ── DrawingBlock ──────────────────────────────────────────────────────────────
// Stores one drawing block extracted from the note file.
struct DrawingBlock {
    QString id;
    QString base64Data; // multi-line base64 payload (newlines preserved)
};

// ── NoteData ──────────────────────────────────────────────────────────────────
// Result of parseNoteFile — plain text + extracted drawing blocks.
struct NoteData {
    QString plainText;
    // Drawing blocks in order of appearance; positions refer to plainText offsets
    QList<QPair<int, DrawingBlock>> drawingPositions; // (textOffset, block)
};

// Parse a .md note file, extracting drawing blocks per contracts/note-format.md
NoteData parseNoteFile(const QString &path);

// ── CNotebook ─────────────────────────────────────────────────────────────────
class CNotebook : public QTextEdit
{
    Q_OBJECT
public:
    explicit CNotebook(QWidget *parent = nullptr);

    // Load a note from disk (T013)
    void openNote(const QString &path);
    // Save the currently loaded note back to disk, re-inserting drawing blocks (T013)
    void saveNote();

    // Current note path (empty if no note loaded)
    QString currentPath() const { return m_currentPath; }

    bool isModified() const;

    // Text search
    bool findText(const QString &text, bool forward, bool skip);

    // Proximity rendering toggle
    void setProximityEnabled(bool enabled);

    // Set font loaded from Qt resource
    void setNoteFont();

    // Active input mode (Text / Draw / Erase) — T027/T028
    void setInputMode(InputMode mode);
    InputMode inputMode() const { return m_inputMode; }

    // Set active drawing color forwarded to overlay (T033/T027)
    void setActiveDrawingColor(const QColor &color);

    // Autosave timer — last-modified timestamp (milliseconds)
    qint64 lastModified() const { return m_lastModified; }
    void resetLastModified() { m_lastModified = 0; }

signals:
    void linkActivated(const QString &url);
    void noteModified();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *event) override; // T027: keep overlay in sync

private slots:
    void onCursorPositionChanged();
    void onContentChanged();

private:
    MarkdownHighlighter *m_highlighter  = nullptr;
    CheckboxTextObject  *m_checkboxObj  = nullptr;
    CImageWidget        *m_imageObj     = nullptr;

    // T027: Drawing overlay for Draw/Erase modes
    CBoundDrawing *m_drawingOverlay  = nullptr;
    QString        m_drawingId;        // ID of the drawing block loaded into m_drawingOverlay
    bool           m_drawingModified = false; // true only after strokes added/erased

    QString  m_currentPath;
    NoteData m_noteData;          // current parsed note data (positions + blocks)
    bool     m_loading = false;   // suppress change signals during load

    InputMode m_inputMode   = InputMode::Text;
    qint64    m_lastModified = 0;

    void rebuildDocument(const NoteData &data);
};
