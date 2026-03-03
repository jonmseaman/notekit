#pragma once
// Migrated from GtkClipboard + GtkTargetList to QClipboard + QMimeData (T016)
// Preserves embedded drawing paste via base64-encoded drawing JSON in QMimeData

#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QTextEdit>

static const char NOTEKIT_MIME[] = "text/notekit-markdown";

// Copy the current selection to the clipboard.
// Sets both text/plain and text/notekit-markdown MIME types.
inline void notebook_copy_clipboard(QTextEdit *editor)
{
    if (!editor->textCursor().hasSelection())
        return;

    QString selectedText = editor->textCursor().selectedText();
    // Replace Qt paragraph separators with newlines
    selectedText.replace(QChar(0x2029), QChar('\n'));

    auto *mime = new QMimeData();
    mime->setText(selectedText);
    mime->setData(QLatin1StringView(NOTEKIT_MIME), selectedText.toUtf8());

    QApplication::clipboard()->setMimeData(mime);
}

// Cut the current selection
inline void notebook_cut_clipboard(QTextEdit *editor)
{
    if (!editor->textCursor().hasSelection())
        return;
    notebook_copy_clipboard(editor);
    editor->textCursor().removeSelectedText();
}

// Paste from clipboard, preferring notekit-markdown over plain text
inline void notebook_paste_clipboard(QTextEdit *editor)
{
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    if (!mime) return;

    QTextCursor cursor = editor->textCursor();
    if (mime->hasFormat(QLatin1StringView(NOTEKIT_MIME))) {
        QString text = QString::fromUtf8(mime->data(QLatin1StringView(NOTEKIT_MIME)));
        cursor.insertText(text);
    } else if (mime->hasText()) {
        cursor.insertText(mime->text());
    }
}
