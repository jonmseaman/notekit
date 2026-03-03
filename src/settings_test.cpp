#include <QtTest>
#include <QCoreApplication>
#include <QSettings>

#include "settings.h"

class TestSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void loadSettings_defaults();
    void roundtrip_allFields();

private:
    void clearSettings();
};

void TestSettings::initTestCase()
{
    // Use a separate application identity so tests don't touch real settings
    QCoreApplication::setOrganizationName(QStringLiteral("NoteKit_TestOrg"));
    QCoreApplication::setApplicationName(QStringLiteral("NoteKit_Test"));
    clearSettings();
}

void TestSettings::cleanupTestCase()
{
    clearSettings();
}

void TestSettings::clearSettings()
{
    QSettings qs;
    qs.clear();
    qs.sync();
}

void TestSettings::loadSettings_defaults()
{
    clearSettings();
    AppSettings s = loadSettings();

    QString expectedBase = QDir::homePath() + QStringLiteral("/Notes");
    QCOMPARE(s.basePath, expectedBase);
    QVERIFY(s.activeDocument.isEmpty());
    QVERIFY(s.sidebarVisible);
    QVERIFY(s.syntaxHighlighting);
    QVERIFY(!s.presentationMode);
    QCOMPARE(s.paletteColors.size(), 8);
    QCOMPARE(s.paletteActive, 0);
}

void TestSettings::roundtrip_allFields()
{
    clearSettings();

    AppSettings original;
    original.basePath          = QStringLiteral("/tmp/test_notes");
    original.activeDocument    = QStringLiteral("/tmp/test_notes/note.md");
    original.sidebarVisible    = false;
    original.presentationMode  = true;
    original.syntaxHighlighting = false;
    original.paletteColors     = {QStringLiteral("#FF0000"), QStringLiteral("#00FF00")};
    original.paletteActive     = 1;
    original.activePalette     = 0;
    original.paletteName       = QStringLiteral("MyPalette");
    original.paletteCount      = 2;

    saveSettings(original);
    AppSettings loaded = loadSettings();

    QCOMPARE(loaded.basePath,           original.basePath);
    QCOMPARE(loaded.activeDocument,     original.activeDocument);
    QCOMPARE(loaded.sidebarVisible,     original.sidebarVisible);
    QCOMPARE(loaded.presentationMode,   original.presentationMode);
    QCOMPARE(loaded.syntaxHighlighting, original.syntaxHighlighting);
    QCOMPARE(loaded.paletteColors,      original.paletteColors);
    QCOMPARE(loaded.paletteActive,      original.paletteActive);
    QCOMPARE(loaded.activePalette,      original.activePalette);
    QCOMPARE(loaded.paletteName,        original.paletteName);
    QCOMPARE(loaded.paletteCount,       original.paletteCount);
}

QTEST_MAIN(TestSettings)
#include "settings_test.moc"
