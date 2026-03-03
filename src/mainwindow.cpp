#include "mainwindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QButtonGroup>
#include <QMenuBar>
#include <QCheckBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QKeySequence>
#include <QShortcut>
#include <QFontDatabase>
#include <QDebug>
#include <QDateTime>

// ── Constructor ───────────────────────────────────────────────────────────────

CMainWindow::CMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Load settings and perform legacy migration if needed
    if (needsLegacyMigration())
        importLegacyJson(m_settings);
    else
        m_settings = loadSettings();

    setWindowTitle(QStringLiteral("NoteKit"));
    setMinimumSize(800, 550);

    // ── Central layout: splitter with sidebar + editor ────────────────────────
    m_splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_splitter);

    // Left pane: navigation tree (T022)
    m_navView  = new QTreeView(m_splitter);
    m_navModel = new CNavigationModel(this);
    m_navView->setModel(m_navModel);
    m_navView->setMinimumWidth(200);
    m_navView->setMaximumWidth(400);
    m_navView->setHeaderHidden(true);
    m_navView->setDragDropMode(QAbstractItemView::DragDrop);
    m_navView->setDefaultDropAction(Qt::MoveAction);
    m_navView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_navView->setEditTriggers(QAbstractItemView::DoubleClicked
                               | QAbstractItemView::EditKeyPressed);

    // Right pane: notebook editor (T017)
    m_notebook = new CNotebook(m_splitter);
    m_notebook->setProximityEnabled(m_settings.syntaxHighlighting);

    m_splitter->addWidget(m_navView);
    m_splitter->addWidget(m_notebook);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({250, 650});

    // ── Sidebar visibility per settings ──────────────────────────────────────
    m_navView->setVisible(m_settings.sidebarVisible);

    // ── Toolbar ───────────────────────────────────────────────────────────────
    setupToolbar();

    // ── Status bar ────────────────────────────────────────────────────────────
    statusBar()->showMessage(QStringLiteral("Ready"));

    // ── Find-in-files dock ────────────────────────────────────────────────────
    setupFindInFiles();

    // ── Menu bar ─────────────────────────────────────────────────────────────
    setupMenu();

    // ── Keyboard shortcuts ────────────────────────────────────────────────────
    // F9: toggle sidebar
    auto *sidebarShortcut = new QShortcut(QKeySequence(Qt::Key_F9), this);
    connect(sidebarShortcut, &QShortcut::activated, this, &CMainWindow::toggleSidebar);

    // Ctrl+S: save
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() {
        if (m_notebook) {
            m_notebook->saveNote();
            statusBar()->showMessage(QStringLiteral("Saved"), 2000);
        }
    });

    // Ctrl+O: open folder
    auto *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, &CMainWindow::openBasePathDialog);

    // Ctrl+Shift+F: find in files
    auto *fifShortcut = new QShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), this);
    connect(fifShortcut, &QShortcut::activated, this, &CMainWindow::showFindInFiles);

    // ── Navigation signals ────────────────────────────────────────────────────
    connect(m_navView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &CMainWindow::onTreeCurrentChanged);

    connect(m_navModel, &CNavigationModel::itemModified,
            this,       &CMainWindow::onItemModified);

    connect(m_navView, &QTreeView::customContextMenuRequested,
            this,      &CMainWindow::onNavContextMenu);

    // ── Notebook signals ──────────────────────────────────────────────────────
    connect(m_notebook, &CNotebook::linkActivated,
            this,       &CMainWindow::onLinkActivated);
    connect(m_notebook, &CNotebook::noteModified,
            this,       &CMainWindow::onNoteModified);

    // ── Autosave timer (5 second idle) ────────────────────────────────────────
    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setInterval(5000);
    connect(m_autosaveTimer, &QTimer::timeout, this, &CMainWindow::onAutosaveTimer);
    m_autosaveTimer->start();

    // ── About dialog ─────────────────────────────────────────────────────────
    m_about = new CAboutDialog(this);

    // ── Restore state ─────────────────────────────────────────────────────────
    restoreWindowState();

    // ── Load note directory ───────────────────────────────────────────────────
    // Create default basePath if it doesn't exist yet
    if (!m_settings.basePath.isEmpty() && !QDir(m_settings.basePath).exists()) {
        QDir().mkpath(m_settings.basePath);
    }
    if (!m_settings.basePath.isEmpty() && QDir(m_settings.basePath).exists()) {
        m_navModel->loadDirectory(m_settings.basePath);
        m_navView->expandToDepth(1);
        // Restore last open document
        if (!m_settings.activeDocument.isEmpty()
            && QFile::exists(m_settings.activeDocument)) {
            openNote(m_settings.activeDocument);
        }
    }
}

CMainWindow::~CMainWindow()
{
}

// ── closeEvent ────────────────────────────────────────────────────────────────

void CMainWindow::closeEvent(QCloseEvent *event)
{
    if (m_notebook) m_notebook->saveNote();
    saveWindowState();

    m_settings.activeDocument = m_notebook ? m_notebook->currentPath() : QString();
    saveSettings(m_settings);

    event->accept();
}

// ── Window state ──────────────────────────────────────────────────────────────

void CMainWindow::saveWindowState()
{
    QSettings qs;
    qs.setValue(QStringLiteral("window/geometry"), saveGeometry());
    qs.setValue(QStringLiteral("window/state"),    saveState());
    if (m_splitter)
        qs.setValue(QStringLiteral("window/splitter"), m_splitter->saveState());
}

void CMainWindow::restoreWindowState()
{
    QSettings qs;
    QByteArray geo = qs.value(QStringLiteral("window/geometry")).toByteArray();
    if (!geo.isEmpty()) restoreGeometry(geo);
    else resize(1000, 650);

    QByteArray st = qs.value(QStringLiteral("window/state")).toByteArray();
    if (!st.isEmpty()) restoreState(st);

    QByteArray spl = qs.value(QStringLiteral("window/splitter")).toByteArray();
    if (!spl.isEmpty() && m_splitter) m_splitter->restoreState(spl);
}

// ── setupToolbar ──────────────────────────────────────────────────────────────

void CMainWindow::setupToolbar()
{
    m_toolbar = new QToolBar(QStringLiteral("Main"), this);
    m_toolbar->setObjectName(QStringLiteral("mainToolbar"));
    m_toolbar->setMovable(false);
    addToolBar(Qt::LeftToolBarArea, m_toolbar);

    // New Note / New Folder buttons
    auto *newNoteBtn = new QToolButton(this);
    newNoteBtn->setText(QStringLiteral("+ Note"));
    newNoteBtn->setToolTip(QStringLiteral("New Note"));
    connect(newNoteBtn, &QToolButton::clicked, this, &CMainWindow::actionNewNote);

    auto *newFolderBtn = new QToolButton(this);
    newFolderBtn->setText(QStringLiteral("+ Folder"));
    newFolderBtn->setToolTip(QStringLiteral("New Folder"));
    connect(newFolderBtn, &QToolButton::clicked, this, &CMainWindow::actionNewFolder);

    m_toolbar->addWidget(newNoteBtn);
    m_toolbar->addWidget(newFolderBtn);
    m_toolbar->addSeparator();

    // Text / Draw / Erase mode buttons
    auto *modeGroup = new QButtonGroup(this);

    auto *textBtn  = new QToolButton(this);
    textBtn->setText(QStringLiteral("Text"));
    textBtn->setCheckable(true);
    textBtn->setChecked(true);

    auto *drawBtn  = new QToolButton(this);
    drawBtn->setText(QStringLiteral("Draw"));
    drawBtn->setCheckable(true);

    auto *eraseBtn = new QToolButton(this);
    eraseBtn->setText(QStringLiteral("Erase"));
    eraseBtn->setCheckable(true);

    modeGroup->addButton(textBtn);
    modeGroup->addButton(drawBtn);
    modeGroup->addButton(eraseBtn);
    modeGroup->setExclusive(true);

    m_toolbar->addWidget(textBtn);
    m_toolbar->addWidget(drawBtn);
    m_toolbar->addWidget(eraseBtn);
    m_toolbar->addSeparator();

    connect(textBtn,  &QToolButton::clicked, this, &CMainWindow::setModeText);
    connect(drawBtn,  &QToolButton::clicked, this, &CMainWindow::setModeDraw);
    connect(eraseBtn, &QToolButton::clicked, this, &CMainWindow::setModeErase);

    // Color swatches from settings
    loadPaletteColors();
}

// ── setupMenu ─────────────────────────────────────────────────────────────────

void CMainWindow::setupMenu()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&Open Notes Folder…"), this,
                        &CMainWindow::openBasePathDialog,
                        QKeySequence::Open);
    fileMenu->addAction(QStringLiteral("&Save"), this, [this]() {
        if (m_notebook) m_notebook->saveNote();
    }, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("&Quit"), qApp, &QApplication::quit,
                        QKeySequence::Quit);

    QMenu *editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(QStringLiteral("&Preferences…"), this,
                        &CMainWindow::openPreferences);

    QMenu *viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    viewMenu->addAction(QStringLiteral("Toggle &Sidebar\tF9"), this,
                        &CMainWindow::toggleSidebar);
    viewMenu->addAction(QStringLiteral("Find in &Files…"), this,
                        &CMainWindow::showFindInFiles,
                        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));

    QMenu *helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("&About NoteKit…"), this,
                        &CMainWindow::showAbout);
}

// ── setupFindInFiles ──────────────────────────────────────────────────────────

void CMainWindow::setupFindInFiles()
{
    m_fif = new CFindInFiles(this);
    connect(m_fif, &CFindInFiles::resultFound,
            this,  &CMainWindow::onFifResultFound);
    connect(m_fif, &CFindInFiles::searchComplete,
            this,  &CMainWindow::onFifSearchComplete);

    m_fifDock = new QDockWidget(QStringLiteral("Find in Files"), this);
    m_fifDock->setObjectName(QStringLiteral("fifDock"));
    m_fifDock->setVisible(false);

    auto *container = new QWidget(m_fifDock);
    auto *layout    = new QVBoxLayout(container);

    auto *inputRow  = new QHBoxLayout();
    m_fifInput      = new QLineEdit(container);
    m_fifInput->setPlaceholderText(QStringLiteral("Search term…"));
    auto *searchBtn = new QPushButton(QStringLiteral("Search"), container);
    inputRow->addWidget(m_fifInput);
    inputRow->addWidget(searchBtn);
    layout->addLayout(inputRow);

    m_fifResults = new QListWidget(container);
    layout->addWidget(m_fifResults);

    container->setLayout(layout);
    m_fifDock->setWidget(container);
    addDockWidget(Qt::BottomDockWidgetArea, m_fifDock);

    connect(searchBtn, &QPushButton::clicked, this, [this]() {
        QString term = m_fifInput->text().trimmed();
        if (term.isEmpty()) return;
        m_fifResults->clear();
        statusBar()->showMessage(QStringLiteral("Searching…"));
        m_fif->startSearch(term, m_settings.basePath);
    });
    connect(m_fifInput, &QLineEdit::returnPressed, searchBtn, &QPushButton::click);
    connect(m_fifResults, &QListWidget::itemActivated,
            this, &CMainWindow::onFifItemActivated);
}

// ── loadPaletteColors ─────────────────────────────────────────────────────────

void CMainWindow::loadPaletteColors()
{
    if (!m_toolbar) return;
    // Add color swatch buttons to the toolbar
    const QStringList &colors = m_settings.paletteColors;
    for (int i = 0; i < colors.size() && i < 8; ++i) {
        auto *btn = new QToolButton(m_toolbar);
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        QPixmap pm(16, 16);
        pm.fill(QColor(colors[i]));
        btn->setIcon(QIcon(pm));
        btn->setToolTip(colors[i]);
        if (i == m_settings.paletteActive) btn->setChecked(true);
        m_toolbar->addWidget(btn);
        // T033: connect swatch to setActiveDrawingColor
        QColor colorValue(colors[i]);
        connect(btn, &QToolButton::clicked, this, [this, colorValue]() {
            if (m_notebook) m_notebook->setActiveDrawingColor(colorValue);
        });
    }
}

// ── Navigation slot ───────────────────────────────────────────────────────────

void CMainWindow::onTreeCurrentChanged(const QModelIndex &current,
                                       const QModelIndex & /*previous*/)
{
    if (!current.isValid()) return;
    int type = current.data(NodeTypeRole).toInt();
    if (type != NodeFile) return;

    QString path = current.data(FullPathRole).toString();
    openNote(path);
}

void CMainWindow::onItemModified(const QString &oldPath, const QString &newPath)
{
    // If the currently open note was renamed or deleted, update
    if (!oldPath.isEmpty() && m_notebook->currentPath() == oldPath) {
        if (newPath.isEmpty()) {
            // File deleted
            m_notebook->openNote(QString());
            setWindowTitle(QStringLiteral("NoteKit"));
        } else {
            // File renamed
            m_notebook->openNote(newPath);
            setWindowTitle(QFileInfo(newPath).baseName()
                           + QStringLiteral(" — NoteKit"));
        }
    }
}

// ── Note opening ─────────────────────────────────────────────────────────────

void CMainWindow::openNote(const QString &path)
{
    if (m_notebook->currentPath() == path) return;
    m_notebook->saveNote(); // save current note before switching
    m_notebook->openNote(path);
    setWindowTitle(QFileInfo(path).baseName() + QStringLiteral(" — NoteKit"));
    statusBar()->showMessage(path);
    m_activeDocument = path;
}

// ── Link handler ─────────────────────────────────────────────────────────────

void CMainWindow::onLinkActivated(const QString &url)
{
    qDebug() << "onLinkActivated:" << url;
    QUrl u(url);
    if (u.scheme().isEmpty() || u.scheme() == QStringLiteral("file")) {
        // Might be a relative note path
        QString target = url;
        if (!QFile::exists(target)) {
            // Resolve relative to current note
            QFileInfo fi(m_notebook->currentPath());
            target = fi.absoluteDir().filePath(url);
        }
        if (QFile::exists(target)) {
            openNote(target);
            return;
        }
    }
    QDesktopServices::openUrl(u);
}

// ── Modified / autosave ───────────────────────────────────────────────────────

void CMainWindow::onNoteModified()
{
    // Window title asterisk
    QString title = windowTitle();
    if (!title.startsWith(QLatin1Char('*')))
        setWindowTitle(QStringLiteral("*") + title);
}

void CMainWindow::onAutosaveTimer()
{
    if (!m_notebook) return;
    qint64 lm = m_notebook->lastModified();
    if (lm > 0) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - lm;
        if (elapsed >= 5000) {
            m_notebook->saveNote();
            // Remove asterisk from title
            QString title = windowTitle();
            if (title.startsWith(QLatin1Char('*')))
                setWindowTitle(title.mid(1));
            statusBar()->showMessage(QStringLiteral("Saved"), 2000);
        }
    }
}

// ── Sidebar toggle ────────────────────────────────────────────────────────────

void CMainWindow::toggleSidebar()
{
    if (!m_navView) return;
    bool visible = !m_navView->isVisible();
    m_navView->setVisible(visible);
    m_settings.sidebarVisible = visible;
}

// ── Open base path dialog ─────────────────────────────────────────────────────

void CMainWindow::openBasePathDialog()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Open Notes Folder"),
        m_settings.basePath.isEmpty() ? QDir::homePath() : m_settings.basePath);

    if (dir.isEmpty()) return;

    m_settings.basePath = dir;
    saveSettings(m_settings);

    m_navModel->loadDirectory(dir);
    m_navView->expandToDepth(1);
    m_notebook->openNote(QString());
    setWindowTitle(QStringLiteral("NoteKit"));
}

// ── Preferences dialog ────────────────────────────────────────────────────────

void CMainWindow::openPreferences()
{
    // Simple preferences dialog (T032)
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Preferences"));
    auto *layout = new QVBoxLayout(&dlg);

    auto *pathRow   = new QHBoxLayout();
    auto *pathLabel = new QLabel(QStringLiteral("Notes folder:"), &dlg);
    auto *pathEdit  = new QLineEdit(m_settings.basePath, &dlg);
    auto *pathBtn   = new QPushButton(QStringLiteral("Browse…"), &dlg);
    pathRow->addWidget(pathLabel);
    pathRow->addWidget(pathEdit);
    pathRow->addWidget(pathBtn);
    layout->addLayout(pathRow);

    auto *hlCheck = new QCheckBox(QStringLiteral("Enable syntax highlighting"), &dlg);
    hlCheck->setChecked(m_settings.syntaxHighlighting);
    layout->addWidget(hlCheck);

    connect(pathBtn, &QPushButton::clicked, this, [&dlg, pathEdit]() {
        QString d = QFileDialog::getExistingDirectory(
            &dlg, QStringLiteral("Notes Folder"), pathEdit->text());
        if (!d.isEmpty()) pathEdit->setText(d);
    });

    auto *btnRow = new QHBoxLayout();
    auto *okBtn  = new QPushButton(QStringLiteral("OK"),     &dlg);
    auto *canBtn = new QPushButton(QStringLiteral("Cancel"), &dlg);
    connect(okBtn,  &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(canBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    btnRow->addStretch();
    btnRow->addWidget(okBtn);
    btnRow->addWidget(canBtn);
    layout->addLayout(btnRow);

    if (dlg.exec() == QDialog::Accepted) {
        QString newPath = pathEdit->text();
        bool    hlNew   = hlCheck->isChecked();

        if (newPath != m_settings.basePath) {
            m_settings.basePath = newPath;
            m_navModel->loadDirectory(newPath);
            m_navView->expandToDepth(1);
        }
        if (hlNew != m_settings.syntaxHighlighting) {
            m_settings.syntaxHighlighting = hlNew;
            m_notebook->setProximityEnabled(hlNew);
        }
        saveSettings(m_settings);
    }
}

// ── About ─────────────────────────────────────────────────────────────────────

void CMainWindow::showAbout()
{
    if (m_about) m_about->exec();
}

// ── Find in files ─────────────────────────────────────────────────────────────

void CMainWindow::showFindInFiles()
{
    if (!m_fifDock) return;
    m_fifDock->setVisible(!m_fifDock->isVisible());
    if (m_fifDock->isVisible() && m_fifInput)
        m_fifInput->setFocus();
}

void CMainWindow::onFifResultFound(const QString &path, const QString &snippet, int line)
{
    if (!m_fifResults) return;
    QFileInfo fi(path);
    QString text = QStringLiteral("%1:%2 — %3")
                   .arg(fi.fileName()).arg(line).arg(snippet);
    auto *item = new QListWidgetItem(text, m_fifResults);
    item->setData(Qt::UserRole, path);
    item->setData(Qt::UserRole + 1, line);
}

void CMainWindow::onFifSearchComplete()
{
    statusBar()->showMessage(
        QStringLiteral("Search complete — %1 result(s)")
        .arg(m_fifResults ? m_fifResults->count() : 0));
}

void CMainWindow::onFifItemActivated(QListWidgetItem *item)
{
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) openNote(path);
}

// ── Navigation context menu ───────────────────────────────────────────────────

void CMainWindow::onNavContextMenu(const QPoint &pos)
{
    QModelIndex idx = m_navView->indexAt(pos);

    QMenu menu(this);
    menu.addAction(QStringLiteral("New Note"),   this, &CMainWindow::actionNewNote);
    menu.addAction(QStringLiteral("New Folder"), this, &CMainWindow::actionNewFolder);

    if (idx.isValid()) {
        menu.addSeparator();
        menu.addAction(QStringLiteral("Rename…"), this, &CMainWindow::actionRename);
        menu.addAction(QStringLiteral("Delete"),  this, &CMainWindow::actionDelete);
    }

    menu.exec(m_navView->viewport()->mapToGlobal(pos));
}

void CMainWindow::actionNewNote()
{
    QModelIndex parent = m_navView->currentIndex();
    // If current is a file, use its parent directory
    if (parent.isValid() && parent.data(NodeTypeRole).toInt() == NodeFile)
        parent = parent.parent();

    QModelIndex newIdx = m_navModel->addNote(parent);
    if (newIdx.isValid()) {
        m_navView->setCurrentIndex(newIdx);
        m_navView->edit(newIdx);
    }
}

void CMainWindow::actionNewFolder()
{
    QModelIndex parent = m_navView->currentIndex();
    if (parent.isValid() && parent.data(NodeTypeRole).toInt() == NodeFile)
        parent = parent.parent();

    QModelIndex newIdx = m_navModel->addFolder(parent);
    if (newIdx.isValid()) {
        m_navView->setCurrentIndex(newIdx);
        m_navView->edit(newIdx);
    }
}

void CMainWindow::actionRename()
{
    QModelIndex idx = m_navView->currentIndex();
    if (!idx.isValid()) return;

    bool ok;
    QString current = idx.data(Qt::DisplayRole).toString();
    QString name = QInputDialog::getText(this, QStringLiteral("Rename"),
                                         QStringLiteral("New name:"),
                                         QLineEdit::Normal, current, &ok);
    if (ok && !name.isEmpty() && name != current)
        m_navModel->renameItem(idx, name);
}

void CMainWindow::actionDelete()
{
    QModelIndex idx = m_navView->currentIndex();
    if (!idx.isValid()) return;

    QString name = idx.data(Qt::DisplayRole).toString();
    int type = idx.data(NodeTypeRole).toInt();
    QString msg = (type == NodeFolder)
        ? QStringLiteral("Delete folder \"%1\" and all its contents?").arg(name)
        : QStringLiteral("Delete note \"%1\"?").arg(name);

    if (QMessageBox::question(this, QStringLiteral("Delete"), msg,
                              QMessageBox::Yes | QMessageBox::No)
        == QMessageBox::Yes) {
        m_navModel->deleteItem(idx);
    }
}

// ── Input mode buttons ────────────────────────────────────────────────────────

void CMainWindow::setModeText()
{
    if (m_notebook) m_notebook->setInputMode(InputMode::Text);
}

void CMainWindow::setModeDraw()
{
    if (m_notebook) m_notebook->setInputMode(InputMode::Draw);
}

void CMainWindow::setModeErase()
{
    if (m_notebook) m_notebook->setInputMode(InputMode::Erase);
}
