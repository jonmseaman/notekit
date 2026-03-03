#pragma once
// Migrated from Gtk::Widget + child anchors to QTextObjectInterface (T015)
// CheckboxTextObject: renders a checkbox glyph as an inline text object

#include "imagewidgets.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QTextEdit>
#include <QMouseEvent>
#include <QAbstractTextDocumentLayout>

// Custom object type for checkboxes
enum { CheckboxObjectType = QTextFormat::UserObject + 3 };
// QTextFormat property key for checked state
enum { CheckboxCheckedProp = QTextFormat::UserProperty + 10 };

// Checkbox size in pixels
static constexpr int CHECKBOX_SIZE = 16;

// ── CheckboxTextObject ────────────────────────────────────────────────────────
class CheckboxTextObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    explicit CheckboxTextObject(QObject *parent = nullptr)
        : QObject(parent) {}

    QSizeF intrinsicSize(QTextDocument * /*doc*/, int /*pos*/,
                         const QTextFormat & /*fmt*/) override
    {
        return QSizeF(CHECKBOX_SIZE, CHECKBOX_SIZE);
    }

    void drawObject(QPainter *painter, const QRectF &rect,
                    QTextDocument * /*doc*/, int /*pos*/,
                    const QTextFormat &format) override
    {
        bool checked = format.property(CheckboxCheckedProp).toBool();

        QStyleOptionButton opt;
        opt.rect    = rect.toRect();
        opt.state   = QStyle::State_Enabled;
        if (checked)
            opt.state |= QStyle::State_On;
        else
            opt.state |= QStyle::State_Off;

        QApplication::style()->drawControl(QStyle::CE_CheckBox, &opt, painter);
    }

    // Make a QTextCharFormat for a checkbox object
    static QTextCharFormat makeFormat(bool checked = false)
    {
        QTextCharFormat fmt;
        fmt.setObjectType(CheckboxObjectType);
        fmt.setProperty(CheckboxCheckedProp, checked);
        return fmt;
    }
};

// ── registerEmbeddedObjects ───────────────────────────────────────────────────
// Call once after QTextDocument is created to register all embedded object handlers.
inline void registerEmbeddedObjects(QTextDocument *doc,
                                    CheckboxTextObject *checkboxHandler,
                                    CImageWidget *imageHandler)
{
    doc->documentLayout()->registerHandler(CheckboxObjectType, checkboxHandler);
    doc->documentLayout()->registerHandler(ImageObjectType,    imageHandler);
#ifdef HAVE_CLATEXMATH
    doc->documentLayout()->registerHandler(LatexObjectType,    imageHandler);
#endif
}

// ── handleMousePress ──────────────────────────────────────────────────────────
// Call from CNotebook::mousePressEvent to toggle checkboxes.
inline bool handleCheckboxClick(QTextEdit *editor, QMouseEvent *event)
{
    // Use QTextEdit's cursorForPosition to find the character under the mouse
    QTextCursor cursor = editor->cursorForPosition(event->pos());
    if (cursor.isNull()) return false;

    QTextCharFormat fmt = cursor.charFormat();
    if (fmt.objectType() != CheckboxObjectType) return false;

    // Toggle checked state — select the object character then replace its format
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    bool wasChecked = fmt.property(CheckboxCheckedProp).toBool();
    fmt.setProperty(CheckboxCheckedProp, !wasChecked);
    cursor.setCharFormat(fmt);
    return true;
}
