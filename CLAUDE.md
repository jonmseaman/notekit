# notekit Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-03-02

## Active Technologies

- **Language**: C++17 minimum; C++23 permitted where all three target compilers (GCC, Clang, MSVC) support it
- **UI**: Qt 6.10 — Widgets, Core, Gui, Concurrent (feature: 001-gtk-to-qt)
- **Dependencies**: jsoncpp, cLaTeXMath (vendored), zlib, fontconfig (Linux only)
- **Storage**: Markdown UTF-8 files with embedded JSON drawing data; QSettings for preferences

## Project Structure

```text
/ (repository root — flat layout)
├── main.cpp, mainwindow.*, notebook.*, navigation.*, drawing.*
├── settings.*, findinfiles.*, imagewidgets.*, about.*
├── data/resources.qrc  (icons, fonts, stylesheet.qss)
└── CMakeLists.txt      (primary build — Qt 6.10)
```

## Commands

```bash
# Configure + build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run
./build/notekit

# Test
ctest --test-dir build --output-on-failure
```

## Code Style

- C++17 minimum; follow existing file conventions
- All Qt signal/slot connections MUST use new-style functor syntax: `connect(sender, &Sender::signal, receiver, &Receiver::slot)`
- No GTK, gtkmm, GtkSourceView, Cairo (direct), or sigc++ includes anywhere
- All assets accessed via Qt resource paths (`:/path/to/asset`), never runtime filesystem paths

## Constitution Principles (summary)

1. **Qt-First UI** — all UI via Qt 6.10 Widgets + QPainter
2. **Multiplatform** — Linux x86_64, Windows x86_64, macOS arm64+x86_64; no ifdefs except where Qt doesn't abstract a platform difference
3. **CMake primary** — `CMakeLists.txt` is the sole build definition; `meson.build` deprecated
4. **Feature parity first** — no new features until all existing GTK3 functionality is reproduced
5. **Minimal deps** — every non-Qt dependency justified in `CMakeLists.txt` comment

## Recent Changes

- 001-gtk-to-qt: Migration from GTK3/GtkSourceView3 to Qt 6.10; CMake replaces Meson as primary build

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
