# Quickstart: NoteKit Qt Build

**Branch**: `001-gtk-to-qt` | **Date**: 2026-03-02

This guide covers setting up a development environment and building NoteKit with its Qt 6.10 UI layer.

---

## Prerequisites

### All Platforms

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| CMake | 4.2 | Required for Qt 6 CMake integration |
| Qt | 6.10 | Components: Core, Gui, Widgets, Concurrent |
| C++ compiler | GCC 12 / Clang 15 / MSVC 2022 | C++17 support required |
| jsoncpp | 1.9+ | Note format serialisation |
| zlib | 1.2+ | Drawing data compression |

### Linux Only

| Tool | Notes |
|------|-------|
| fontconfig | Font resolution |
| pkg-config | Dependency discovery |

---

## Install Dependencies

### Ubuntu / Debian

```bash
sudo apt install cmake ninja-build qt6-base-dev qt6-base-dev-tools \
    libzlib-dev libjsoncpp-dev libfontconfig1-dev pkg-config
```

### Arch Linux

```bash
sudo pacman -S cmake ninja qt6-base jsoncpp zlib fontconfig
```

### macOS (Homebrew)

```bash
brew install cmake ninja qt@6 jsoncpp
# Qt 6 formula may need PATH adjustment:
export PATH="$(brew --prefix qt@6)/bin:$PATH"
```

### Windows (vcpkg)

```powershell
vcpkg install qt6-base jsoncpp zlib
```

Or install Qt via the [Qt Online Installer](https://www.qt.io/download) and set `CMAKE_PREFIX_PATH` to the Qt installation directory.

---

## Build

```bash
# Configure
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.10.0/gcc_64  # adjust for your platform

# Build
cmake --build build

# Run
./build/notekit
```

### Qt Creator (Recommended)

1. Open `CMakeLists.txt` in Qt Creator.
2. Qt Creator auto-detects the Qt 6.10 kit.
3. Press **Run** (Ctrl+R).

---

## Verify Installation

```bash
# Should print "NoteKit 0.3.0" and open the main window
./build/notekit --version
```

---

## cLaTeXMath (Optional — LaTeX Math Rendering)

cLaTeXMath is vendored as a submodule. To enable LaTeX math rendering:

```bash
git submodule update --init --recursive

# Configure with LaTeX support
cmake -B build -G Ninja -DWITH_LATEX=ON ...
```

If `WITH_LATEX` is `OFF`, inline LaTeX expressions are displayed as plain text.

---

## Running Tests

```bash
cmake --build build --target check
# or
ctest --test-dir build --output-on-failure
```

---

## Key Source Files

| File | Purpose |
|------|---------|
| `main.cpp` | `QApplication` entry point |
| `mainwindow.h/cpp` | `QMainWindow` — top-level window and layout |
| `notebook.h/cpp` | `QTextEdit`-based markdown editor with drawing overlay |
| `navigation.h/cpp` | `QStandardItemModel` + `QTreeView` sidebar |
| `drawing.h/cpp` | `QWidget`-based drawing canvas (`QPainter`) |
| `settings.h/cpp` | `QSettings` wrapper with legacy JSON migration |
| `findinfiles.h/cpp` | `QThread`-based cross-note search |
| `data/resources.qrc` | Qt resource file (icons, fonts, stylesheet) |

---

## Architecture Notes

- **Drawing overlay**: `CBoundDrawing` is a `QWidget` child of `CNotebook`, positioned as an overlay over the text area. It receives `QTabletEvent` and `QMouseEvent` when draw/erase mode is active.
- **Embedded objects**: Checkboxes and LaTeX images inside notes are rendered via `QTextObjectInterface`; see `notebook_widgets.hpp`.
- **Proximity rendering**: The `MarkdownHighlighter` class (in `notebook_highlight.hpp`) re-highlights blocks near the cursor on `cursorPositionChanged`.
- **Thread safety**: `CFindInFiles` runs on a `QThread`; results are delivered to the main thread via Qt's `Qt::QueuedConnection` signal/slot mechanism — no manual mutex needed in the GUI layer.
