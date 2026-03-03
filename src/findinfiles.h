#pragma once
// Migrated from Glib::Dispatcher + std::thread to QThread + Qt signals (T029)
// Thread-safe result delivery via Qt::QueuedConnection (replaces Glib::Dispatcher)

#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>

// Worker that runs in a QThread
class FindWorker : public QObject
{
    Q_OBJECT
public:
    explicit FindWorker(QObject *parent = nullptr);

public slots:
    void search(const QString &term, const QString &basePath);
    void cancel();

signals:
    void resultFound(const QString &path, const QString &contextSnippet, int lineNumber);
    void searchComplete();

private:
    std::atomic<bool> m_stop{false};

    void searchDir(const QString &basePath, const QString &dirPath, const QString &term);
    void searchFile(const QString &filePath, const QString &relPath, const QString &term);
};

// Public API — wraps the worker thread
class CFindInFiles : public QObject
{
    Q_OBJECT
public:
    explicit CFindInFiles(QObject *parent = nullptr);
    ~CFindInFiles() override;

public slots:
    void startSearch(const QString &term, const QString &basePath);
    void cancelSearch();

signals:
    // Delivered via Qt::QueuedConnection from worker thread to main thread
    void resultFound(const QString &path, const QString &contextSnippet, int lineNumber);
    void searchComplete();

private:
    QThread   *m_thread  = nullptr;
    FindWorker *m_worker = nullptr;
};
