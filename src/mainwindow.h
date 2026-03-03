#pragma once
// Migrated from Gtk::ApplicationWindow to QMainWindow (T006/T007)
// Phase 3 integration: CNotebook as right pane (T017)
// Phase 4 integration: CNavigationModel + QTreeView as left pane (T022)

#include "config.h"
#include "settings.h"
#include "notebook.h"
#include "navigation.h"
#include "findinfiles.h"
#include "about.h"

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QTimer>
#include <QSettings>
#include <QCloseEvent>
#include <QStringList>

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit CMainWindow(QWidget *parent = nullptr);
    ~CMainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void saveWindowState();
    void restoreWindowState();

    void openBasePathDialog();
    void openNote(const QString &path);
    void onTreeCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void onItemModified(const QString &oldPath, const QString &newPath);
    void onLinkActivated(const QString &url);
    void onNoteModified();
    void onAutosaveTimer();

    void actionNewNote();
    void actionNewFolder();
    void actionRename();
    void actionDelete();
    void toggleSidebar();
    void openPreferences();
    void showAbout();
    void showFindInFiles();
    void onFifResultFound(const QString &path, const QString &snippet, int line);
    void onFifSearchComplete();
    void onFifItemActivated(QListWidgetItem *item);

    void onNavContextMenu(const QPoint &pos);

    // Input mode toolbar buttons
    void setModeText();
    void setModeDraw();
    void setModeErase();

private:
    void setupMenu();
    void setupToolbar();
    void setupFindInFiles();
    void loadPaletteColors();

    AppSettings m_settings;

    // Layout
    QSplitter          *m_splitter    = nullptr;
    QTreeView          *m_navView     = nullptr;
    CNotebook          *m_notebook    = nullptr;
    CNavigationModel   *m_navModel    = nullptr;

    // Toolbar
    QToolBar           *m_toolbar     = nullptr;

    // Find in files dock
    QDockWidget        *m_fifDock     = nullptr;
    QLineEdit          *m_fifInput    = nullptr;
    QListWidget        *m_fifResults  = nullptr;
    CFindInFiles       *m_fif         = nullptr;

    // About dialog
    CAboutDialog       *m_about       = nullptr;

    // Autosave timer (5s idle)
    QTimer             *m_autosaveTimer = nullptr;

    QString m_activeDocument;
    bool    m_binit = true; // true during first base-path setup
};
