#include "findinfiles.h"
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QDebug>

// ---------- FindWorker -------------------------------------------------------

FindWorker::FindWorker(QObject *parent)
    : QObject(parent)
{}

void FindWorker::cancel()
{
    m_stop.store(true);
}

void FindWorker::search(const QString &term, const QString &basePath)
{
    m_stop.store(false);
    searchDir(basePath, basePath, term);
    emit searchComplete();
}

void FindWorker::searchDir(const QString &basePath, const QString &dirPath, const QString &term)
{
    QDirIterator it(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        if (m_stop.load()) return;
        QString entry = it.next();
        QFileInfo fi(entry);
        if (fi.fileName().startsWith(QLatin1Char('.')))
            continue; // skip dotfiles
        if (fi.isDir()) {
            searchDir(basePath, entry, term);
        } else if (fi.suffix().compare(QStringLiteral("md"), Qt::CaseInsensitive) == 0) {
            searchFile(entry, fi.filePath(), term);
        }
    }
}

void FindWorker::searchFile(const QString &filePath, const QString &relPath, const QString &term)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream ts(&file);
    int lineNum = 0;
    while (!ts.atEnd()) {
        if (m_stop.load()) return;
        QString line = ts.readLine();
        ++lineNum;
        int pos = line.indexOf(term, 0, Qt::CaseInsensitive);
        if (pos >= 0) {
            QString snippet = line.trimmed();
            if (snippet.length() > 120)
                snippet = snippet.left(117) + QStringLiteral("...");
            emit resultFound(filePath, snippet, lineNum);
        }
    }
}

// ---------- CFindInFiles -----------------------------------------------------

CFindInFiles::CFindInFiles(QObject *parent)
    : QObject(parent)
{}

CFindInFiles::~CFindInFiles()
{
    cancelSearch();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void CFindInFiles::startSearch(const QString &term, const QString &basePath)
{
    // Cancel any running search
    cancelSearch();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
        m_worker = nullptr;
    }

    m_thread = new QThread(this);
    m_worker = new FindWorker();
    m_worker->moveToThread(m_thread);

    // Worker → CFindInFiles: Qt::QueuedConnection crosses thread boundary
    connect(m_worker, &FindWorker::resultFound,
            this,     &CFindInFiles::resultFound,
            Qt::QueuedConnection);
    connect(m_worker, &FindWorker::searchComplete,
            this,     &CFindInFiles::searchComplete,
            Qt::QueuedConnection);
    connect(m_worker, &FindWorker::searchComplete,
            m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished,
            m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::started,
            m_worker, [this, term, basePath]() {
                m_worker->search(term, basePath);
            });

    m_thread->start();
}

void CFindInFiles::cancelSearch()
{
    if (m_worker)
        m_worker->cancel();
}
