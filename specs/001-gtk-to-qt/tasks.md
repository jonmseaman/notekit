# Tasks: GTK3 to Qt Migration

**Input**: Design documents from `/specs/001-gtk-to-qt/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/ ✅, quickstart.md ✅

**Tests**: No test tasks — not requested in spec. Tests added opportunistically per module (serialisation, format parsing) where regressions are highest risk.

**Organization**: Tasks are grouped by user story. Each phase delivers a testable increment. Phase 2 (Foundational) must complete before any user story work begins.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no shared state)
- **[Story]**: User story label (US1–US5)
- Flat source layout per plan.md — all files at repository root

---

## Phase 1: Setup (Build System & Assets)

**Purpose**: Get a Qt 6.10 CMake build compiling before any GTK code is touched. Meson is deprecated in place. Assets are registered in the Qt resource system.

- [x] T001 Update `CMakeLists.txt` — replace all `gtkmm-3.0`, `gtksourceviewmm-3.0`, `gdk-x11-3.0` pkg-config deps with `find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)`; retain `jsoncpp`, `zlib`, `fontconfig` (Linux-only, guarded with `if(UNIX AND NOT APPLE)`); switch from `add_executable` to `qt_add_executable`; add `qt_add_resources(notekit "resources" FILES data/resources.qrc)` in `CMakeLists.txt`
- [x] T002 [P] Add deprecation comment block at top of `meson.build` — `# DEPRECATED: This build file is superseded by CMakeLists.txt (Qt 6.10). Do not use for new builds. Will be removed when GTK3 migration is complete.`
- [x] T003 [P] Create `data/resources.qrc` — Qt resource manifest registering all entries in `data/fonts/`, `data/icons/`, `data/default_config.json`, and `data/stylesheet.qss` under the prefix `/notekit/`
- [x] T004 [P] Convert `data/stylesheet.css` to `data/stylesheet.qss` — translate each GTK widget selector to its Qt equivalent (e.g. `GtkTreeView` → `QTreeView`, `GtkTextView` → `QTextEdit`, `GtkHeaderBar` → `QMainWindow > QToolBar`); preserve all color and sizing rules; keep original `stylesheet.css` until GTK build is fully retired

**Checkpoint**: `cmake -B build -G Ninja && cmake --build build` succeeds (may not link until Phase 2 stub is in place).

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Minimal running Qt application — QApplication + QMainWindow opens, loads stylesheet and settings. No user-facing features yet. Must be complete before any user story begins.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [x] T005 Migrate `main.cpp` — replace `Gtk::Application::create` and `Gsv::init()` with `QApplication app(argc, argv)`; set application name/version/organisation; load `:/notekit/stylesheet.qss` via `QFile` and apply with `app.setStyleSheet()`; instantiate `CMainWindow` and call `show()`; remove all `#include <gtkmm.h>` and GtkSourceView includes from `main.cpp`
- [x] T006 Scaffold `mainwindow.h` — replace `class CMainWindow : public Gtk::ApplicationWindow` with `class CMainWindow : public QMainWindow`; add `Q_OBJECT` macro; replace all `Gtk::*`/`Glib::*`/`sigc::*` member types with Qt equivalents: `QSplitter* m_splitter`, `QTreeView* m_navView` (stub), `QWidget* m_editorPane` (stub), `QToolBar* m_toolbar`, `QStatusBar* m_statusBar`; declare `void saveWindowState()` and `void restoreWindowState()` slots; remove all GTK includes from `mainwindow.h`
- [x] T007 Scaffold `mainwindow.cpp` — implement `CMainWindow` constructor: create `QSplitter(Qt::Horizontal)` with stub `QTreeView` (250px) and stub `QTextEdit` as right pane; create `QToolBar` with placeholder `QAction` entries; call `restoreWindowState()` from constructor; implement `saveWindowState()` / `restoreWindowState()` using `QSettings` (`window/geometry`, `window/state`); use new-style `connect()` throughout; remove all GTK includes from `mainwindow.cpp`
- [x] T008 [P] Migrate `settings.h` — replace `Gio::Settings` with `QSettings`; declare `AppSettings` struct matching all keys from `contracts/settings-schema.md`; declare `AppSettings load()`, `void save(const AppSettings&)`, `bool needsLegacyMigration()`, `void importLegacyJson(AppSettings&)` in `settings.h`; remove all GLib/Gio includes
- [x] T009 [P] Migrate `settings.cpp` — implement `load()` reading all keys from `contracts/settings-schema.md` via `QSettings` with documented defaults; implement `save()` writing all keys; implement `importLegacyJson()` reading the legacy JSON config file from XDG/AppData path per `contracts/settings-schema.md §Migration`, mapping legacy keys, and setting `migration/legacy-json-imported=true` in `QSettings`; do not modify or delete the legacy JSON file; remove all GLib/Gio/GSettings includes from `settings.cpp`

**Checkpoint**: `cmake --build build && ./build/notekit` — application window opens with toolbar and empty tree/editor panes, window size restores on relaunch. No GTK includes in any compiled file.

---

## Phase 3: User Story 1 — Full Note Editing Experience (Priority: P1) 🎯 MVP

**Goal**: User opens a note directory; existing notes load and display with markdown formatting and LaTeX math. Text editing and saving work. Note format is byte-for-byte identical to the GTK3 version.

**Independent Test**: Launch the Qt build pointing at an existing GTK3 note directory. Open several notes. Verify markdown formatting renders, LaTeX math renders (if WITH_LATEX=ON), and saving a note produces a file indistinguishable from what the GTK3 version would produce.

### Implementation

- [x] T010 [P] [US1] Implement note file parser in `notebook.cpp` — function `parseNoteFile(QString path)` reads a `.md` file, extracts all `<drawing data-for="ID">…</drawing>` blocks per `contracts/note-format.md`, returns a struct containing: plain text with drawing markers replaced by placeholder cursor positions, and a `QMap<QString, QByteArray>` of raw compressed drawing payloads keyed by ID; add corresponding declaration in `notebook.h`
- [x] T011 [P] [US1] Implement `MarkdownHighlighter` in `notebook_highlight.hpp` — `QSyntaxHighlighter` subclass with `QTextCharFormat` rules for: ATX headings (`#`–`######`), bold (`**`/`__`), italic (`*`/`_`), inline code (`` ` ``), fenced code blocks (` ``` `), blockquotes (`>`), links (`[text](url)`), horizontal rules; all rules defined as `QRegularExpression` patterns with associated `QTextCharFormat`; add `proximityRehighlight(int blockNumber)` slot called on `cursorPositionChanged` to toggle markup visibility within ±2 blocks of cursor via `QTextCharFormat::setForeground(Qt::transparent)` in `notebook_highlight.hpp`
- [x] T012 [US1] Migrate `notebook.h` — replace `class CNotebook : public Gsv::View` with `class CNotebook : public QTextEdit`; add `Q_OBJECT` macro; replace all `Gsv::*`/`Gtk::TextTag`/`Cairo::*`/`Gdk::Cursor` member types with: `QTextDocument* m_doc`, `MarkdownHighlighter* m_highlighter`, `QMap<QString, QByteArray> m_drawingPayloads` (loaded drawing data), `QString m_currentPath`; preserve `signal_link` as Qt signal `void linkActivated(QString url)`; remove all GTK/Cairo/GLib includes from `notebook.h`
- [x] T013 [US1] Implement `CNotebook` constructor and text editing core in `notebook.cpp` — set up `QTextDocument`, attach `MarkdownHighlighter`; set monospace font loaded from `:/notekit/fonts/`; connect `cursorPositionChanged` to `m_highlighter->proximityRehighlight`; implement `openNote(QString path)` calling parser from T010, loading plain text into `QTextDocument`, storing drawing payloads; implement `saveNote()` re-inserting drawing blocks at saved positions into plain text before `QFile::write`; remove all GTK/GLib/Cairo includes from `notebook.cpp`
- [x] T014 [P] [US1] Migrate `imagewidgets.h/cpp` — replace `CImageWidget : public Gtk::DrawingArea` with `CImageWidget : public QObject, public QTextObjectInterface`; implement `intrinsicSize(QTextDocument*, int, const QTextFormat&)` returning pixmap dimensions; implement `drawObject(QPainter*, const QRectF&, QTextDocument*, int, const QTextFormat&)` rendering the pixmap via `QPainter`; if `WITH_LATEX=ON`, implement `CLatexWidget` generating a `QPixmap` via cLaTeXMath; remove all Cairo/GTK includes from `imagewidgets.h` and `imagewidgets.cpp`
- [x] T015 [US1] Implement embedded widget handling in `notebook_widgets.hpp` — implement `CheckboxTextObject : QObject, QTextObjectInterface` rendering a `QStyle`-drawn checkbox at text baseline; implement `registerEmbeddedObjects(QTextDocument*)` that registers `CheckboxTextObject` and `CImageWidget` with `QTextDocument::documentLayout()->registerHandler()`; implement `handleMousePress(QTextEdit*, QMouseEvent*)` detecting clicks on checkbox objects via `document()->objectAtPosition()` and toggling checked state; add embedded object insertion helpers for checkbox and LaTeX math in `notebook_widgets.hpp`
- [x] T016 [US1] Migrate `notebook_clipboard.hpp` — replace `GtkClipboard`/`GtkTargetList` APIs with `QClipboard` + `QMimeData`; implement `copySelection(QTextEdit*)` setting both `text/plain` and custom `text/notekit-markdown` MIME types on `QApplication::clipboard()`; implement `pasteAtCursor(QTextEdit*)` preferring `text/notekit-markdown` over `text/plain` when available; preserve embedded drawing paste via base64-encoded drawing JSON in `QMimeData` in `notebook_clipboard.hpp`
- [x] T017 [US1] Integrate `CNotebook` into `mainwindow.cpp` — replace stub `QTextEdit` right pane with `CNotebook`; add `QFileDialog`-based folder-open action (`Ctrl+O`) that sets the notes base directory in `QSettings` and triggers tree load; wire `CNotebook::linkActivated` to `QDesktopServices::openUrl`; add save action (`Ctrl+S`) calling `m_notebook->saveNote()`; connect `CNotebook` load/save status to `QStatusBar` in `mainwindow.cpp`

**Checkpoint Phase 3**: Build and launch → open an existing note directory via File menu → select a note → verify markdown formatting renders → edit and save → open the saved file in a text editor and confirm drawing blocks are intact and identical to GTK3 output.

---

## Phase 4: User Story 3 — Note Navigation and Organization (Priority: P2)

**Goal**: User can navigate their note hierarchy in the sidebar, create/rename/move/delete notes and folders, with all changes reflected on the filesystem.

**Independent Test**: Open app, observe folder tree, right-click to create a note, double-click to rename it, drag it to another folder, right-click to delete a folder — verify filesystem reflects all changes and the tree stays consistent.

### Implementation

- [x] T018 [P] [US3] Migrate `navigation.h` — replace `CNavigationTreeStore : public Gtk::TreeStore` with `CNavigationModel : public QStandardItemModel`; add `Q_OBJECT` macro; define custom `Qt::ItemDataRole` constants: `FullPathRole = Qt::UserRole+1`, `NodeTypeRole = Qt::UserRole+2`, `SortOrderRole = Qt::UserRole+3`; declare `loadDirectory(QString path)`, `addNote(QModelIndex parent)`, `addFolder(QModelIndex parent)`, `renameItem(QModelIndex, QString)`, `deleteItem(QModelIndex)` methods; remove all GTK includes from `navigation.h`
- [x] T019 [US3] Implement `CNavigationModel` in `navigation.cpp` — implement `loadDirectory(QString path)` building `QStandardItem` tree from `QDir` recursively (folders first, files second, sorted by `SortOrderRole`); store `FullPathRole` = absolute path, `NodeTypeRole` = File|Folder, display name in `Qt::DisplayRole`; implement lazy-load by connecting `QTreeView::expanded` signal to load children on first expand; remove all GTK includes from `navigation.cpp`
- [x] T020 [P] [US3] Implement CRUD operations in `navigation.cpp` — `addNote(parent)`: create `.md` file on disk, insert `QStandardItem` under parent with inline editing started; `addFolder(parent)`: `QDir::mkdir`, insert item; `renameItem(index, name)`: `QFile::rename` on disk, update `Qt::DisplayRole` and `FullPathRole`; `deleteItem(index)`: `QDir::removeRecursively` for folders or `QFile::remove` for files, `removeRow` from model; each operation emits `itemModified(QString oldPath, QString newPath)` signal
- [x] T021 [P] [US3] Implement drag-and-drop in `navigation.cpp` — override `supportedDropActions()` returning `Qt::MoveAction`; override `canDropMimeData(data, action, row, col, parent)` checking target is a Folder node; override `dropMimeData(data, action, row, col, parent)` moving the file/directory with `QFile::rename`, updating `FullPathRole` and parent/child structure in model; update `SortOrderRole` of siblings at the destination to reflect new position
- [x] T022 [US3] Integrate `CNavigationModel` + `QTreeView` into `mainwindow.cpp` — replace stub `QTreeView` with `QTreeView` using `CNavigationModel`; call `model->loadDirectory(settings.basePath)` at startup; connect `QTreeView::selectionModel()->currentChanged` to open selected note in `CNotebook`; add right-click `QMenu` with actions: New Note, New Folder, Rename, Delete — connected to model methods from T020; persist sidebar splitter position in `QSettings`; add F9 keyboard shortcut to toggle sidebar visibility in `mainwindow.cpp`

**Checkpoint Phase 4**: Full sidebar interaction — tree loads, notes open on click, CRUD operations work, drag-and-drop moves files.

---

## Phase 5: User Story 2 — Drawing and Pen Annotation (Priority: P3)

**Goal**: User can draw freehand strokes on a note with pressure sensitivity, erase strokes, and have all annotations persist across save/reload. Drawing data is byte-compatible with the GTK3 format.

**Independent Test**: Open a note, switch to draw mode, draw 3–5 strokes with mouse (or stylus), switch to erase mode, erase one stroke, save, close, reopen — verify the 2–4 remaining strokes appear exactly as drawn. Open the same note in the GTK3 build and verify drawings are visible.

### Implementation

- [x] T023 [P] [US2] Migrate `drawing.h` — replace `CBoundDrawing : public Gtk::DrawingArea` with `CBoundDrawing : public QWidget`; add `Q_OBJECT` macro; replace `Cairo::Surface`/`Cairo::Context` members with `QPixmap m_cache` (rendered backing store); replace `Gdk::Window` with `QWindow*`; keep `CStroke` as a pure data struct (no GTK types); replace `sigc::signal` stroke events with Qt signals `void strokeAdded()`, `void strokeErased()`; remove all GTK/Cairo/Gdk includes from `drawing.h`
- [x] T024 [US2] Implement `CBoundDrawing` rendering in `drawing.cpp` — implement `paintEvent(QPaintEvent*)` that draws `m_cache` pixmap then renders the current in-progress stroke with `QPainter` (for low-latency preview); implement `rebuildCache()` iterating all `m_strokes`, calling `renderStroke(QPainter&, CStroke&)` which sets pen width to `stroke.width * stroke.pressures[i]` and draws `QPainterPath` through stroke points; implement bucket spatial index rebuild after stroke add/erase; remove all Cairo/GTK includes from `drawing.cpp`
- [x] T025 [P] [US2] Implement input handling in `drawing.cpp` — override `tabletEvent(QTabletEvent*)` for stylus: on `QEventPoint::State::Pressed` begin stroke, on `Updated` append point+pressure, on `Released` finalise stroke; override `mousePressEvent`/`mouseMoveEvent`/`mouseReleaseEvent` as mouse fallback with fixed pressure 1.0; implement `eraseAtPoint(QPointF, float radius)` using bucket index to find strokes within radius and remove them; emit `strokeAdded()`/`strokeErased()` after each operation
- [x] T026 [US2] Implement drawing serialisation in `drawing.cpp` — implement `toJson()` producing JSON per `contracts/note-format.md §Drawing JSON Schema` (version, width, height, strokes array with color/#RRGGBB, width, points[x,y,p]); implement `compress()` applying raw zlib deflate (not `QByteArray::qCompress` — see `research.md §5`) and base64-encoding; implement `fromJson(QByteArray)` as the inverse; these methods are called by `CNotebook::saveNote()` and `parseNoteFile()` respectively
- [x] T027 [US2] Integrate `CBoundDrawing` overlay into `CNotebook` in `notebook.cpp` — add `CBoundDrawing* m_drawingOverlay` child widget; position overlay to cover the `QTextEdit` viewport via `resizeEvent`; in `Text` mode: hide overlay, pass events to `QTextEdit`; in `Draw`/`Erase` modes: show overlay, forward `QTabletEvent`/`QMouseEvent` to overlay; connect `m_drawingOverlay::strokeAdded` to mark note modified; when `openNote()` loads a note, call `CBoundDrawing::fromJson()` for each drawing block at the correct vertical scroll position in `notebook.cpp`
- [x] T028 [P] [US2] Implement mode switching and cursors in `notebook.cpp` — add `enum InputMode { Text, Draw, Erase }` and `setMode(InputMode)` slot; in `setMode`: update `m_drawingOverlay` visibility and `QWidget::setCursor` (Text → `Qt::IBeamCursor`, Draw → `QCursor(QPixmap(":/notekit/icons/pen-cursor.svg"))`, Erase → circle cursor); expose `setMode` as a slot connected to toolbar buttons in `mainwindow.cpp`

**Checkpoint Phase 5**: Draw strokes, save, reopen — strokes persist. Open saved file in GTK3 build — drawings visible. Stylus pressure varies stroke width (if hardware available).

---

## Phase 6: User Story 4 — Find in Files (Priority: P4)

**Goal**: User can search text across all notes and navigate directly to matching notes from the result list.

**Independent Test**: Launch app, press Ctrl+Shift+F, enter a word present in at least two notes, verify both notes appear in results with context snippets, click one — verify that note opens.

### Implementation

- [x] T029 [P] [US4] Migrate `findinfiles.h` — replace `Glib::Dispatcher` and `std::thread` with `QObject`-based design; define `class CFindInFiles : public QObject` with signals `resultFound(QString path, QString contextSnippet, int lineNumber)` and `searchComplete()`; define `void startSearch(QString term, QString basePath)` and `void cancelSearch()` slots; use `QThread` worker pattern (worker object moved to thread, not `QThread` subclass); remove all GLib includes from `findinfiles.h`
- [x] T030 [US4] Implement `CFindInFiles` in `findinfiles.cpp` — implement `FindWorker : public QObject` with `void search(QString term, QString basePath)` slot; iterate `.md` files under `basePath` via `QDirIterator`; for each file, read lines and emit `resultFound(path, contextSnippet, lineNum)` per match via `Qt::QueuedConnection`; check `m_stop` atomic flag between files for cancellation; emit `searchComplete()` when done; `CFindInFiles::startSearch` moves worker to new `QThread`, connects signals, starts thread; remove all GLib includes from `findinfiles.cpp`
- [x] T031 [US4] Implement find-in-files panel in `mainwindow.cpp` — add `QDockWidget` (bottom or right) containing: `QLineEdit` search input + `QPushButton` Search + `QListWidget` results; instantiate `CFindInFiles`; connect `searchButton::clicked` to `m_fif->startSearch(term, basePath)`; connect `CFindInFiles::resultFound` to append a `QListWidgetItem` with path + snippet text; connect `QListWidget::itemActivated` to open the note in `CNotebook` and scroll to the matched line; add `Ctrl+Shift+F` shortcut toggling dock visibility; clear results list on each new search in `mainwindow.cpp`

**Checkpoint Phase 6**: Ctrl+Shift+F opens panel, search returns results across multiple notes, clicking result opens correct note.

---

## Phase 7: User Story 5 — Settings and Preferences (Priority: P5)

**Goal**: User's existing color palettes and preferences carry over to the Qt build without manual reconfiguration.

**Independent Test**: Copy an existing GTK3 `config.json` to the expected platform path, launch the Qt build for the first time, verify base path is pre-filled, color palettes are loaded, and no manual re-entry is needed.

### Implementation

- [x] T032 [P] [US5] Implement preferences dialog in `mainwindow.cpp` — add `void openPreferences()` slot triggered by Edit > Preferences; build `QDialog` with: base-path `QLineEdit` + `QPushButton` Browse (`QFileDialog::getExistingDirectory`); syntax highlighting `QCheckBox`; color palette section with 8 `QToolButton` swatches, each opening `QColorDialog` on click; OK/Cancel buttons; on Accept, write all values to `QSettings` and refresh toolbar swatches and `CNotebook` highlighter state in `mainwindow.cpp`
- [x] T033 [P] [US5] Implement color palette toolbar in `mainwindow.cpp` — replace GTK `Gtk::RadioToolButton` swatches with a `QButtonGroup` of `QToolButton` widgets in `QToolBar`; load colors from `QSettings palette/0/colors` on startup; clicking a swatch calls `m_notebook->setActiveDrawingColor(color)`; expose `refreshPalette()` slot called after preferences dialog closes; persist `palette/0/active` index in `QSettings` on swatch click

**Checkpoint Phase 7**: Cold launch with legacy config in place — app opens with correct base path and color palette. Hot launch (after migration) — settings load from QSettings without touching legacy file.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Final compliance, cleanup, and multi-platform verification.

- [x] T035 [P] Implement `about.h/cpp` — replace `Gtk::AboutDialog` with `class CAboutDialog : public QDialog`; show app icon from `:/notekit/icons/`, application name "NoteKit", version from `CMakeLists.txt` via `PROJECT_VERSION` preprocessor define, description, license (GPL), project URL as `QLabel` with `linkActivated`; triggered from Help > About in `mainwindow.cpp`; remove all GTK includes from `about.h` and `about.cpp`
- [x] T036 [P] Replace `base64.h` usage — replace all calls to custom `base64.h` encode/decode in `drawing.cpp` and `notebook.cpp` with `QByteArray::toBase64(QByteArray::Base64Encoding)` and `QByteArray::fromBase64`; verify output is identical for the same input data; delete `base64.h` after all usages are replaced
- [x] T037 [P] Principle I compliance audit — run `grep -r '#include.*gtk\|#include.*gsv\|#include.*glib\|#include.*gdk\|#include.*sigc\|#include.*cairo' *.h *.cpp *.hpp` from repo root; each match is a migration error; fix all occurrences; document zero remaining GTK includes in a code comment in `CMakeLists.txt`
- [x] T038 [P] Principle II compliance audit — run `grep -rn '#ifdef\|#if defined' *.h *.cpp *.hpp` from repo root; review each platform-specific block; add a comment `// Platform-specific: Qt does not abstract [X] because [Y]` to each justified block; remove any unjustified `#ifdef` blocks
- [x] T039 Remove GTK-only assets — delete `data/preferences.glade`, `data/toolbar.glade`, `data/gschemas.compiled` from the repository; update `CMakeLists.txt` to remove any references to these files; commit the deletions
- [ ] T041 [P] Update `specs/001-gtk-to-qt/quickstart.md` — update CMake configure command with actual `CMAKE_PREFIX_PATH` values for each platform; add any platform-specific install steps discovered during T040; add a "Known Issues" section for any failures found during T040

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately; T002–T004 can run in parallel with T001
- **Phase 2 (Foundational)**: Depends on Phase 1 completion — BLOCKS all user stories; T008–T009 can run in parallel with T005–T007
- **Phase 3 (US1)**: Depends on Phase 2 — T010/T011/T014 can run in parallel; T013 depends on T010+T011; T015 depends on T014; T016–T017 depend on T013
- **Phase 4 (US3)**: Depends on Phase 2 — T018/T020/T021 can run in parallel; T019 depends on T018; T022 depends on T019+T020+T021
- **Phase 5 (US2)**: Depends on Phase 2 + Phase 3 (US1); T023/T025 can run in parallel; T024 depends on T023; T026 depends on T024; T027–T028 depend on T026
- **Phase 6 (US4)**: Depends on Phase 2 — T029 can start in parallel with T030; T031 depends on T030
- **Phase 7 (US5)**: Depends on Phase 2; T032/T033 can run in parallel; T034 depends on T008+T009
- **Phase 8 (Polish)**: Depends on Phase 3–7 completion; all tasks except T041 can run in parallel with each other

### User Story Dependencies

- **US1 (P1)**: Depends on Foundational only — no dependency on US2–US5
- **US3 (P2)**: Depends on Foundational only — no dependency on US1/US2; can start in parallel with US1 after Phase 2
- **US2 (P3)**: Depends on Foundational + US1 (drawing overlay integrates into `CNotebook`)
- **US4 (P4)**: Depends on Foundational only — no dependency on US1/US2/US3
- **US5 (P5)**: Depends on Foundational (settings already in place) — preferences UI integrates with US1 (highlighter) and US2 (palette)

### Parallel Opportunities Per Phase

**Phase 1**: T002, T003, T004 all parallelise with T001 (different files)
**Phase 2**: T008+T009 parallelise with T005+T006+T007 (settings vs main window scaffold)
**Phase 3**: T010+T011+T014 parallelise (parser, highlighter, image widgets — different files)
**Phase 4**: T018+T020+T021 parallelise (model header, CRUD, drag-drop — different concerns)
**Phase 5**: T023+T025 parallelise (drawing.h migration + input handling stubs — different files)
**Phase 6**: T029+T030 parallelise (header + worker — different files)
**Phase 7**: T032+T033+T034 parallelise (different UI panels)
**Phase 8**: T035–T039 all parallelise (about dialog, base64, audits, cleanup)

---

## Parallel Example: Phase 4 (US3)

```bash
# Launch all parallelisable Phase 4 tasks together:
Task A: "T018 — Migrate navigation.h to QStandardItemModel"
Task B: "T020 — Implement CRUD operations in navigation.cpp"
Task C: "T021 — Implement drag-and-drop in navigation.cpp"

# Then run sequentially:
T019 (depends on T018) → T022 (depends on T019 + T020 + T021)
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T004)
2. Complete Phase 2: Foundational (T005–T009)
3. Complete Phase 3: User Story 1 (T010–T017)
4. **STOP and VALIDATE**: Open an existing note directory, verify markdown editing and note format compatibility
5. Ship Qt build to early adopters for US1 validation

### Incremental Delivery

1. Setup + Foundational → Qt window opens (no features)
2. Add US1 → text editing works → **Demo: markdown editor parity**
3. Add US3 → navigation works → **Demo: full note management** ← promoted
4. Add US2 → drawing works → **Demo: pen annotation parity**
5. Add US4 → search works → **Demo: find-in-files**
6. Add US5 → settings carry over → **Demo: zero-friction migration**
7. Polish → clean compliance + multi-platform → **Ship**

### Suggested Parallel Team Strategy

With 2–3 developers after Phase 2:
- **Dev A**: US1 (P1) — notebook, highlighter, format parsing
- **Dev B**: US3 (P2) — navigation model and sidebar (fully independent of US1)
- **Dev C**: US4 (P4) — find-in-files (fully independent)
- US2 follows US1 (overlay integrates into CNotebook); US5 follows US2+US3

---

## Notes

- `[P]` tasks touch different files — safe to run in parallel with no merge conflicts
- `[Story]` label maps each task to its user story for traceability to spec.md acceptance scenarios
- Each phase ends with a **Checkpoint** — validate independently before proceeding
- All Qt signal/slot connections MUST use new-style functor syntax per constitution
- No GTK, gtkmm, GLib, sigc++, or direct Cairo `#include` may appear in any file after its migration task is marked complete
- zlib used directly (not `QByteArray::qCompress`) — see `research.md §5` for rationale
- Drawing overlay integration (T027) is the highest-risk task — allocate extra time
- Embedded widget objects (T015) — test with checkbox notes from GTK3 build early
