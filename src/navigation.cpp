#include "navigation.h"

#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QMimeData>
#include <QStringList>
#include <QDataStream>
#include <QDebug>
#include <functional>

// ── CNavigationModel ──────────────────────────────────────────────────────────

CNavigationModel::CNavigationModel(QObject *parent)
    : QStandardItemModel(parent)
{
    setHorizontalHeaderLabels({QStringLiteral("Name")});
}

void CNavigationModel::loadDirectory(const QString &path)
{
    clear();
    setHorizontalHeaderLabels({QStringLiteral("Name")});
    m_basePath = path;

    if (path.isEmpty()) return;

    QDir dir(path);
    if (!dir.exists()) {
        qWarning() << "CNavigationModel::loadDirectory: path does not exist:" << path;
        return;
    }

    populateItem(invisibleRootItem(), path);
}

void CNavigationModel::populateItem(QStandardItem *parent, const QString &dirPath)
{
    QDir dir(dirPath);

    // Folders first, then files — both sorted alphabetically
    QFileInfoList entries = dir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name);

    int sortOrder = 0;
    for (const QFileInfo &fi : std::as_const(entries)) {
        if (fi.fileName().startsWith(QLatin1Char('.')))
            continue; // skip hidden files

        NodeType type = fi.isDir() ? NodeFolder : NodeFile;

        // Only show .md files and directories
        if (!fi.isDir() && fi.suffix().compare(QStringLiteral("md"), Qt::CaseInsensitive) != 0)
            continue;

        // For .md files, show filename without extension
        QString displayName = fi.isDir() ? fi.fileName() : fi.completeBaseName();
        if (displayName.isEmpty()) displayName = fi.fileName();

        QStandardItem *item = makeItem(displayName, fi.absoluteFilePath(), type);
        item->setData(sortOrder++, SortOrderRole);

        parent->appendRow(item);

        if (fi.isDir())
            populateItem(item, fi.absoluteFilePath());
    }
}

QStandardItem *CNavigationModel::makeItem(const QString &name,
                                           const QString &fullPath,
                                           NodeType type)
{
    auto *item = new QStandardItem(name);
    item->setData(fullPath, FullPathRole);
    item->setData(type,     NodeTypeRole);
    item->setEditable(true);

    // Choose icon based on type
    if (type == NodeFolder)
        item->setIcon(QIcon::fromTheme(QStringLiteral("folder")));
    else
        item->setIcon(QIcon::fromTheme(QStringLiteral("text-x-generic")));

    return item;
}

QString CNavigationModel::itemFullPath(const QStandardItem *item)
{
    return item ? item->data(FullPathRole).toString() : QString();
}

// ── CRUD operations (T020) ────────────────────────────────────────────────────

QModelIndex CNavigationModel::addNote(const QModelIndex &parent)
{
    QStandardItem *parentItem = parent.isValid()
                                ? itemFromIndex(parent)
                                : invisibleRootItem();

    QString parentPath = parent.isValid()
                         ? parentItem->data(FullPathRole).toString()
                         : m_basePath;

    // Find an unused filename
    QString newName = QStringLiteral("New Note");
    QString newPath = parentPath + QDir::separator() + newName + QStringLiteral(".md");
    int n = 1;
    while (QFile::exists(newPath)) {
        newName = QStringLiteral("New Note %1").arg(++n);
        newPath = parentPath + QDir::separator() + newName + QStringLiteral(".md");
    }

    // Create file on disk
    QFile f(newPath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "addNote: cannot create file" << newPath;
        return {};
    }
    f.close();

    // Insert into model
    QStandardItem *item = makeItem(newName, newPath, NodeFile);
    item->setData(parentItem->rowCount(), SortOrderRole);
    parentItem->appendRow(item);

    QModelIndex idx = indexFromItem(item);
    emit itemModified(QString(), newPath);
    return idx;
}

QModelIndex CNavigationModel::addFolder(const QModelIndex &parent)
{
    QStandardItem *parentItem = parent.isValid()
                                ? itemFromIndex(parent)
                                : invisibleRootItem();

    QString parentPath = parent.isValid()
                         ? parentItem->data(FullPathRole).toString()
                         : m_basePath;

    QString newName = QStringLiteral("New Folder");
    QString newPath = parentPath + QDir::separator() + newName;
    int n = 1;
    while (QDir(newPath).exists()) {
        newName = QStringLiteral("New Folder %1").arg(++n);
        newPath = parentPath + QDir::separator() + newName;
    }

    if (!QDir().mkdir(newPath)) {
        qWarning() << "addFolder: cannot create directory" << newPath;
        return {};
    }

    QStandardItem *item = makeItem(newName, newPath, NodeFolder);
    item->setData(parentItem->rowCount(), SortOrderRole);
    parentItem->appendRow(item);

    QModelIndex idx = indexFromItem(item);
    emit itemModified(QString(), newPath);
    return idx;
}

void CNavigationModel::renameItem(const QModelIndex &index, const QString &newName)
{
    if (!index.isValid()) return;
    QStandardItem *item = itemFromIndex(index);
    if (!item) return;

    QString oldPath = item->data(FullPathRole).toString();
    QFileInfo fi(oldPath);

    QString newPath;
    if (item->data(NodeTypeRole).toInt() == NodeFile) {
        newPath = fi.absolutePath() + QDir::separator() + newName + QStringLiteral(".md");
    } else {
        newPath = fi.absolutePath() + QDir::separator() + newName;
    }

    if (!QFile::rename(oldPath, newPath)) {
        qWarning() << "renameItem: cannot rename" << oldPath << "to" << newPath;
        return;
    }

    item->setText(newName);
    item->setData(newPath, FullPathRole);

    // Update full paths of all children recursively
    std::function<void(QStandardItem *, const QString &, const QString &)> updatePaths;
    updatePaths = [&](QStandardItem *it, const QString &oldBase, const QString &newBase) {
        for (int r = 0; r < it->rowCount(); ++r) {
            QStandardItem *child = it->child(r);
            if (!child) continue;
            QString childOld = child->data(FullPathRole).toString();
            QString childNew = newBase + childOld.mid(oldBase.length());
            child->setData(childNew, FullPathRole);
            updatePaths(child, oldBase, newBase);
        }
    };
    if (item->data(NodeTypeRole).toInt() == NodeFolder)
        updatePaths(item, oldPath, newPath);

    emit itemModified(oldPath, newPath);
}

void CNavigationModel::deleteItem(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QStandardItem *item = itemFromIndex(index);
    if (!item) return;

    QString path = item->data(FullPathRole).toString();
    bool ok;
    if (item->data(NodeTypeRole).toInt() == NodeFolder) {
        ok = QDir(path).removeRecursively();
    } else {
        ok = QFile::remove(path);
    }

    if (!ok) {
        qWarning() << "deleteItem: cannot remove" << path;
        return;
    }

    QStandardItem *parentItem = item->parent() ? item->parent() : invisibleRootItem();
    parentItem->removeRow(item->row());

    emit itemModified(path, QString());
}

// ── Drag-and-drop (T021) ──────────────────────────────────────────────────────

Qt::DropActions CNavigationModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool CNavigationModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                        int /*row*/, int /*column*/,
                                        const QModelIndex &parent) const
{
    if (action != Qt::MoveAction) return false;
    if (!data->hasFormat(QStringLiteral("application/x-qabstractitemmodeldatalist")))
        return false;

    // Can only drop onto a folder node (or root)
    if (parent.isValid()) {
        int type = parent.data(NodeTypeRole).toInt();
        if (type != NodeFolder) return false;
    }
    return true;
}

bool CNavigationModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                     int row, int column,
                                     const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    // Decode the dragged item's path from MIME data
    QByteArray encoded = data->data(
        QStringLiteral("application/x-qabstractitemmodeldatalist"));
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
        int srcRow, srcCol;
        QMap<int, QVariant> roleData;
        stream >> srcRow >> srcCol >> roleData;

        QString srcPath = roleData.value(FullPathRole).toString();
        if (srcPath.isEmpty()) continue;

        // Determine destination directory
        QString dstDir = parent.isValid()
                         ? parent.data(FullPathRole).toString()
                         : m_basePath;

        QFileInfo fi(srcPath);
        QString dstPath = dstDir + QDir::separator() + fi.fileName();
        if (srcPath == dstPath) continue;

        if (!QFile::rename(srcPath, dstPath)) {
            qWarning() << "dropMimeData: cannot move" << srcPath << "to" << dstPath;
            continue;
        }

        emit itemModified(srcPath, dstPath);
    }

    // Reload the full tree to reflect changes
    loadDirectory(m_basePath);
    return true;
}
