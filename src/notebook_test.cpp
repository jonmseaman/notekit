#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

#include "notebook.h"

// On macOS, NSTemporaryDirectory() ignores TMPDIR; use an explicit writable path.
static QTemporaryDir makeTempDir()
{
    // Try project-relative path that is known to be writable in all environments.
    QString base = QDir::currentPath() + QStringLiteral("/test_tmp");
    QDir().mkpath(base);
    return QTemporaryDir(base + QStringLiteral("/XXXXXX"));
}

class TestNotebook : public QObject
{
    Q_OBJECT

private slots:
    void parseNoteFile_missingFile();
    void parseNoteFile_emptyFile();
    void parseNoteFile_plainText();
    void parseNoteFile_withDrawingBlock();
    void roundtrip_preservesDrawingBlock();
};

void TestNotebook::parseNoteFile_missingFile()
{
    NoteData data = parseNoteFile(QStringLiteral("/nonexistent/path/does_not_exist.md"));
    QVERIFY(data.plainText.isEmpty());
    QVERIFY(data.drawingPositions.isEmpty());
}

void TestNotebook::parseNoteFile_emptyFile()
{
    auto dir = makeTempDir();
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QStringLiteral("empty.md"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();

    NoteData data = parseNoteFile(path);
    QVERIFY(data.plainText.isEmpty());
    QVERIFY(data.drawingPositions.isEmpty());
}

void TestNotebook::parseNoteFile_plainText()
{
    auto dir = makeTempDir();
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QStringLiteral("plain.md"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << QStringLiteral("# Hello\n\nSome text.\n");
    f.close();

    NoteData data = parseNoteFile(path);
    QCOMPARE(data.plainText, QStringLiteral("# Hello\n\nSome text.\n"));
    QVERIFY(data.drawingPositions.isEmpty());
}

void TestNotebook::parseNoteFile_withDrawingBlock()
{
    auto dir = makeTempDir();
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QStringLiteral("drawing.md"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << QStringLiteral("Before\n")
       << QStringLiteral("<drawing data-for=\"sketch1\">AAAA\nBBBB\n</drawing>\n")
       << QStringLiteral("After\n");
    f.close();

    NoteData data = parseNoteFile(path);
    QCOMPARE(data.drawingPositions.size(), 1);
    QCOMPARE(data.drawingPositions[0].second.id, QStringLiteral("sketch1"));
    QCOMPARE(data.drawingPositions[0].second.base64Data, QStringLiteral("AAAA\nBBBB\n"));
    // Plain text should contain "Before\n" and "After\n" but not the drawing block
    QVERIFY(data.plainText.contains(QStringLiteral("Before")));
    QVERIFY(data.plainText.contains(QStringLiteral("After")));
    QVERIFY(!data.plainText.contains(QStringLiteral("<drawing")));
}

void TestNotebook::roundtrip_preservesDrawingBlock()
{
    auto dir = makeTempDir();
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QStringLiteral("roundtrip.md"));
    QString drawingId = QStringLiteral("draw1");
    QString drawingData = QStringLiteral("SGVsbG8gV29ybGQ=\n");

    // Write initial file
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << QStringLiteral("# Title\n\n")
       << QStringLiteral("<drawing data-for=\"") << drawingId << QStringLiteral("\">")
       << drawingData
       << QStringLiteral("</drawing>\n")
       << QStringLiteral("Footer\n");
    f.close();

    // Open in CNotebook and save back
    CNotebook nb;
    nb.openNote(path);
    nb.saveNote();

    // Re-parse and verify drawing block survived
    NoteData data = parseNoteFile(path);
    QCOMPARE(data.drawingPositions.size(), 1);
    QCOMPARE(data.drawingPositions[0].second.id, drawingId);
    QCOMPARE(data.drawingPositions[0].second.base64Data, drawingData);
}

QTEST_MAIN(TestNotebook)
#include "notebook_test.moc"
