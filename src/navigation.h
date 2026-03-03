#pragma once
// Migrated from Gtk::TreeStore + Gtk::TreeView to QStandardItemModel + QTreeView (T018)
// Custom ItemDataRole constants per tasks.md T018:
//   FullPathRole  = Qt::UserRole + 1
//   NodeTypeRole  = Qt::UserRole + 2
//   SortOrderRole = Qt::UserRole + 3

#include <QStandardItemModel>
#include <QStandardItem>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QMimeData>

// Node types stored in NodeTypeRole
enum NodeType { NodeFile = 0, NodeFolder = 1 };

// Custom role constants
enum NavigationRole {
    FullPathRole  = Qt::UserRole + 1,
    NodeTypeRole  = Qt::UserRole + 2,
    SortOrderRole = Qt::UserRole + 3,
};

// ── CNavigationModel ──────────────────────────────────────────────────────────
class CNavigationModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit CNavigationModel(QObject *parent = nullptr);

    QString basePath() const { return m_basePath; }

    // Load all notes/folders from basePath (T019)
    void loadDirectory(const QString &path);

    // CRUD operations (T020)
    QModelIndex addNote(const QModelIndex &parent);
    QModelIndex addFolder(const QModelIndex &parent);
    void renameItem(const QModelIndex &index, const QString &name);
    void deleteItem(const QModelIndex &index);

    // Drag-and-drop (T021)
    Qt::DropActions supportedDropActions() const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                         int row, int column,
                         const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column,
                      const QModelIndex &parent) override;

signals:
    void itemModified(const QString &oldPath, const QString &newPath);

private:
    QString m_basePath;

    void populateItem(QStandardItem *parent, const QString &dirPath);

    static QString itemFullPath(const QStandardItem *item);
    static QStandardItem *makeItem(const QString &name,
                                   const QString &fullPath,
                                   NodeType type);
};
