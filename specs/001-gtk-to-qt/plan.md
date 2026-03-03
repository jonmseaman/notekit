# Implementation Plan: GTK3 to Qt Migration

**Branch**: `001-gtk-to-qt` | **Date**: 2026-03-02 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-gtk-to-qt/spec.md`

## Summary

Migrate NoteKit's GTK3/GtkSourceView3 UI layer to Qt 6.10, replacing all GTK, Cairo, GLib, and sigc++ dependencies with Qt equivalents while maintaining 100% note-file format compatibility and feature parity across Linux, Windows, and macOS. The build system transitions from Meson (current primary) to CMake (required per constitution), with `CMakeLists.txt` updated for Qt 6.10. Migration proceeds module-by-module in dependency order, keeping the application in a buildable state throughout.

## Technical Context

**Language/Version**: C++17 minimum; C++23 permitted where all three target compilers (GCC, Clang, MSVC) support it
**Primary Dependencies**: Qt 6.10 (Widgets, Core, Gui, Concurrent), jsoncpp, cLaTeXMath (vendored), zlib, fontconfig (Linux only)
**Storage**: Markdown files (UTF-8 plain text with embedded JSON drawing data); QSettings for application preferences
**Testing**: QTest (Qt built-in); unit tests for drawing serialisation and note-format parsing added during migration
**Target Platform**: Linux x86_64, Windows x86_64, macOS arm64 + x86_64
**Project Type**: Desktop application (single-user, single-process, file-based storage)
**Performance Goals**: Startup to first interactive window within 20% of GTK3 baseline; drawing canvas at 60fps+ during active stylus input
**Constraints**: Existing note file format must remain byte-for-byte compatible; no user data migration step; no new user-facing features until FR-001–FR-013 are all met
**Scale/Scope**: Single-user desktop; typical note collections of hundreds to thousands of Markdown files; no network I/O

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Qt-First UI | ✅ PASS | All GTK, gtkmm, GtkSourceView, direct Cairo, and sigc++ dependencies eliminated. All UI implemented via Qt 6.10 Widgets + QPainter. Custom drawing canvas uses QWidget + paintEvent. |
| II. Multiplatform by Default | ✅ PASS | CMakeLists.txt targets Linux x86_64, Windows x86_64, macOS arm64+x86_64. fontconfig guarded under `if(UNIX AND NOT APPLE)`. No other platform ifdefs expected. |
| III. Qt Creator / CMake Build System | ✅ PASS | CMakeLists.txt updated to be the sole primary build file. `meson.build` deprecated in place with a comment during the transition period, removed when migration is complete. |
| IV. Feature Parity Before Extension | ✅ PASS | Spec FR-001–FR-013 enumerate all existing user-facing features. No new functionality is in scope until parity is verified. |
| V. Minimal Dependency Footprint | ✅ PASS | Non-Qt runtime deps: jsoncpp (note format), zlib (drawing compression, format compatibility), fontconfig (Linux font resolution only), cLaTeXMath (vendored). All available and maintained on all three platforms. |

No violations — Complexity Tracking omitted.

## Project Structure

### Documentation (this feature)

```text
specs/001-gtk-to-qt/
├── plan.md              # This file (/speckit.plan output)
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   ├── note-format.md   # On-disk note + drawing serialisation contract
│   └── settings-schema.md  # Application settings contract
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created here)
```

### Source Code (repository root)

Migration keeps the existing flat source layout to allow file-by-file migration without structural churn. GTK `.glade` files are eliminated; toolbar and preferences UI are built in C++ or compiled via Qt Designer `.ui` files bundled in `.qrc`.

```text
/ (repository root)
├── main.cpp
├── mainwindow.h / mainwindow.cpp
├── notebook.h / notebook.cpp
├── notebook_clipboard.hpp
├── notebook_highlight.hpp
├── notebook_widgets.hpp
├── navigation.h / navigation.cpp
├── drawing.h / drawing.cpp
├── imagewidgets.h / imagewidgets.cpp
├── settings.h / settings.cpp
├── findinfiles.h / findinfiles.cpp
├── about.h / about.cpp
├── base64.h
├── data/
│   ├── fonts/             # Custom fonts (retained as-is)
│   ├── icons/             # Application icons (migrated to .qrc)
│   ├── stylesheet.qss     # Qt stylesheet (replaces GTK stylesheet.css)
│   ├── default_config.json  # Default settings template (retained)
│   └── resources.qrc      # Qt resource manifest (new)
├── CMakeLists.txt         # Primary build (updated for Qt 6.10)
└── meson.build            # DEPRECATED — comment added, removed at migration complete
```

**Structure Decision**: Flat layout retained. Each source file is migrated independently; no directory reorganisation until after feature parity is achieved.

## Migration Module Order

Modules migrate in dependency order. Each completed module leaves the application in a buildable (and where possible runnable) state.

| Order | Module | Removed GTK Dependency | Qt Replacement | Risk |
|-------|--------|------------------------|----------------|------|
| 1 | Build system | meson.build + GTK CMake deps | CMakeLists.txt with Qt 6.10 | Low |
| 2 | `settings` | `Gio::Settings`, `GSettings` C API | `QSettings` | Low |
| 3 | `findinfiles` | `Glib::Dispatcher`, `std::thread` | `QThread` + Qt signals | Low |
| 4 | `base64` | (none) | Retain or replace with `QByteArray::toBase64` | Trivial |
| 5 | `drawing` | `Cairo`, `Gtk::DrawingArea`, `GdkDevice` | `QPainter`, `QWidget`, `QTabletEvent` | Medium |
| 6 | `imagewidgets` | `Cairo`, `Gtk::DrawingArea` | `QPainter`, `QWidget` | Medium |
| 7 | `navigation` | `Gtk::TreeStore`, `Gtk::TreeView` | `QStandardItemModel`, `QTreeView` | Medium |
| 8 | `notebook` | `Gsv::View`, `Gsv::Buffer`, Cairo overlay | `QTextEdit`, `QSyntaxHighlighter`, `QPainter` overlay | High |
| 9 | `about` | `Gtk::AboutDialog` | `QDialog` | Trivial |
| 10 | `mainwindow` | `Gtk::ApplicationWindow`, all layouts | `QMainWindow`, Qt layouts + toolbars | High |
| 11 | `main.cpp` | `Gtk::Application`, `Gsv::init()` | `QApplication` | Low |
| 12 | Assets | `.glade` files, GTK CSS | `.qrc`, `.qss` stylesheet | Medium |

## Post-Design Constitution Check

*Re-evaluated after Phase 1 design artifacts (research.md, data-model.md, contracts/).*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Qt-First UI | ✅ PASS | All design decisions use Qt APIs exclusively. Drawing canvas: QWidget + QPainter. Editor: QTextEdit + QSyntaxHighlighter. No Cairo or GTK type appears in any design artifact. |
| II. Multiplatform by Default | ✅ PASS | QTabletEvent replaces GdkDevice (Qt abstracts platform pressure input). fontconfig isolated to Linux. Qt resource system eliminates runtime path lookup. |
| III. Qt Creator / CMake Build System | ✅ PASS | CMakeLists.txt with `find_package(Qt6 REQUIRED COMPONENTS ...)` is the complete build definition. |
| IV. Feature Parity Before Extension | ✅ PASS | data-model.md and contracts map 1:1 to existing GTK behaviour. No additions. |
| V. Minimal Dependency Footprint | ✅ PASS | zlib retained (not replaced by `QByteArray::qCompress`) to preserve byte-for-byte format compatibility — see research.md §5. |
