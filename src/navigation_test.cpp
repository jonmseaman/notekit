#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QSignalSpy>

#include "navigation.h"

static QTemporaryDir makeTempDir()
{
    QString base = QDir::currentPath() + QStringLiteral("/test_tmp");
    QDir().mkpath(base);
    return QTemporaryDir(base + QStringLiteral("/XXXXXX"));
}

class TestNavigation : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void addNote_createsFileAndModelItem();
    void addFolder_createsDirAndModelItem();
    void renameItem_renamesFileAndUpdatesPath();
    void deleteItem_removesFileAndModelRow();

private:
    QTemporaryDir *m_dir = nullptr;
    CNavigationModel *m_model = nullptr;
};

void TestNavigation::init()
{
    QString base = QDir::currentPath() + QStringLiteral("/test_tmp");
    QDir().mkpath(base);
    m_dir = new QTemporaryDir(base + QStringLiteral("/XXXXXX"));
    QVERIFY(m_dir->isValid());
    m_model = new CNavigationModel();
    m_model->loadDirectory(m_dir->path());
}

void TestNavigation::cleanup()
{
    delete m_model;
    m_model = nullptr;
    delete m_dir;
    m_dir = nullptr;
}

void TestNavigation::addNote_createsFileAndModelItem()
{
    int rowsBefore = m_model->rowCount();
    QModelIndex newIdx = m_model->addNote(QModelIndex());

    QVERIFY(newIdx.isValid());
    QCOMPARE(m_model->rowCount(), rowsBefore + 1);

    QString path = newIdx.data(FullPathRole).toString();
    QVERIFY(!path.isEmpty());
    QVERIFY(QFile::exists(path));
    QVERIFY(path.endsWith(QStringLiteral(".md")));
    QCOMPARE(newIdx.data(NodeTypeRole).toInt(), (int)NodeFile);
}

void TestNavigation::addFolder_createsDirAndModelItem()
{
    int rowsBefore = m_model->rowCount();
    QModelIndex newIdx = m_model->addFolder(QModelIndex());

    QVERIFY(newIdx.isValid());
    QCOMPARE(m_model->rowCount(), rowsBefore + 1);

    QString path = newIdx.data(FullPathRole).toString();
    QVERIFY(!path.isEmpty());
    QVERIFY(QDir(path).exists());
    QCOMPARE(newIdx.data(NodeTypeRole).toInt(), (int)NodeFolder);
}

void TestNavigation::renameItem_renamesFileAndUpdatesPath()
{
    QModelIndex noteIdx = m_model->addNote(QModelIndex());
    QVERIFY(noteIdx.isValid());

    QString oldPath = noteIdx.data(FullPathRole).toString();
    QVERIFY(QFile::exists(oldPath));

    m_model->renameItem(noteIdx, QStringLiteral("renamed_note"));

    // Old path should no longer exist; new path should exist
    QVERIFY(!QFile::exists(oldPath));
    QString newPath = noteIdx.data(FullPathRole).toString();
    QVERIFY(QFile::exists(newPath));
    QVERIFY(newPath.endsWith(QStringLiteral("renamed_note.md")));
}

void TestNavigation::deleteItem_removesFileAndModelRow()
{
    QModelIndex noteIdx = m_model->addNote(QModelIndex());
    QVERIFY(noteIdx.isValid());
    QString path = noteIdx.data(FullPathRole).toString();
    QVERIFY(QFile::exists(path));

    int rowsBefore = m_model->rowCount();
    m_model->deleteItem(noteIdx);

    QVERIFY(!QFile::exists(path));
    QCOMPARE(m_model->rowCount(), rowsBefore - 1);
}

int run_navigation_tests(int argc, char **argv)
{
    TestNavigation t;
    return QTest::qExec(&t, argc, argv);
}
#include "navigation_test.moc"
