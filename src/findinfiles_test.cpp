#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include "findinfiles.h"

static QTemporaryDir makeTempDir()
{
    QString base = QDir::currentPath() + QStringLiteral("/test_tmp");
    QDir().mkpath(base);
    return QTemporaryDir(base + QStringLiteral("/XXXXXX"));
}

class TestFindInFiles : public QObject
{
    Q_OBJECT

private slots:
    void search_findsMatchingFile();
    void search_noMatchEmitsComplete();
    void search_multipleMatches();
};

static void writeFile(const QString &path, const QString &content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << content;
}

void TestFindInFiles::search_findsMatchingFile()
{
    auto dir = makeTempDir();
    QVERIFY(dir.isValid());
    writeFile(dir.filePath(QStringLiteral("note.md")),
              QStringLiteral("This contains the needle\nAnother line\n"));

    CFindInFiles fif;
    QSignalSpy resultSpy(&fif, &CFindInFiles::resultFound);
    QSignalSpy completeSpy(&fif, &CFindInFiles::searchComplete);

    fif.startSearch(QStringLiteral("needle"), dir.path());

    // Wait up to 3s for search to complete
    QVERIFY(completeSpy.wait(3000));
    QCOMPARE(resultSpy.count(), 1);

    QString foundPath = resultSpy[0][0].toString();
    QVERIFY(foundPath.endsWith(QStringLiteral("note.md")));

    QString snippet = resultSpy[0][1].toString();
    QVERIFY(snippet.contains(QStringLiteral("needle"), Qt::CaseInsensitive));
}

void TestFindInFiles::search_noMatchEmitsComplete()
{
    auto dir = makeTempDir();
    QVERIFY(dir.isValid());
    writeFile(dir.filePath(QStringLiteral("note.md")),
              QStringLiteral("Nothing useful here\nJust some text\n"));

    CFindInFiles fif;
    QSignalSpy resultSpy(&fif, &CFindInFiles::resultFound);
    QSignalSpy completeSpy(&fif, &CFindInFiles::searchComplete);

    fif.startSearch(QStringLiteral("xyzzy_not_found"), dir.path());

    QVERIFY(completeSpy.wait(3000));
    QCOMPARE(resultSpy.count(), 0);
    QCOMPARE(completeSpy.count(), 1);
}

void TestFindInFiles::search_multipleMatches()
{
    auto dir = makeTempDir();
    QVERIFY(dir.isValid());
    writeFile(dir.filePath(QStringLiteral("a.md")),
              QStringLiteral("foo line one\nfoo line two\n"));
    writeFile(dir.filePath(QStringLiteral("b.md")),
              QStringLiteral("nothing here\n"));

    CFindInFiles fif;
    QSignalSpy resultSpy(&fif, &CFindInFiles::resultFound);
    QSignalSpy completeSpy(&fif, &CFindInFiles::searchComplete);

    fif.startSearch(QStringLiteral("foo"), dir.path());

    QVERIFY(completeSpy.wait(3000));
    // Both lines in a.md should match
    QCOMPARE(resultSpy.count(), 2);
}

int run_findinfiles_tests(int argc, char **argv)
{
    TestFindInFiles t;
    return QTest::qExec(&t, argc, argv);
}
#include "findinfiles_test.moc"
