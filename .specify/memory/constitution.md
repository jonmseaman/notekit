<!--
SYNC IMPACT REPORT
==================
Version change: [TEMPLATE] → 1.0.0
Bump rationale: MINOR — initial population of all placeholder tokens; first ratified version.

Added sections:
  - Core Principles (5 principles: Qt-First UI, Multiplatform, QtCreator/CMake,
    Feature Parity, Minimal Dependency Footprint)
  - Technology Stack
  - Migration Standards
  - Governance

Modified principles: N/A (initial population from template)
Removed sections: N/A (template comments stripped)

Templates requiring updates:
  - .specify/templates/plan-template.md  ✅ no update needed (Constitution Check is generic)
  - .specify/templates/spec-template.md  ✅ no update needed (generic)
  - .specify/templates/tasks-template.md ✅ no update needed (generic)

Deferred TODOs: None
-->

# NoteKit Constitution

## Core Principles

### I. Qt-First User Interface

All user interface code MUST be implemented using Qt 6.10 widgets and APIs. No GTK,
gtkmm, GtkSourceView, or other non-Qt UI toolkit dependencies are permitted in new or
migrated code. Custom rendering (e.g., the drawing canvas) MUST use QPainter, QWidget,
or QGraphicsView — never platform-native drawing APIs called directly outside Qt's
abstraction layer.

**Rationale**: Standardising on Qt 6.10 enables consistent cross-platform UI behaviour,
eliminates GTK platform-specific quirks, and allows the team to use Qt Creator tooling
for design, debugging, and profiling.

### II. Multiplatform by Default

All code MUST compile and run correctly on Linux, Windows, and macOS without
platform-specific `#ifdef` guards except where the underlying Qt API itself does not
abstract a platform difference. Every platform-specific code block MUST include a comment
justifying why Qt's abstraction is insufficient for that case.

**Rationale**: NoteKit's value proposition is being a free, platform-independent
notetaking application. All three platforms MUST receive first-class, equally tested
support.

### III. Qt Creator / CMake Build System

The project MUST be buildable via Qt Creator using CMake as the build system. The root
`CMakeLists.txt` MUST define all targets, dependencies, and install rules. Meson and
qmake build files MUST NOT be retained as primary build definitions after migration is
complete — they may be kept temporarily with a clear deprecation comment during the
transition period only.

**Rationale**: Qt Creator + CMake is the officially supported Qt 6 build workflow and
provides the best IDE integration, CMake presets support, and CI reproducibility for the
target development environment.

### IV. Feature Parity Before Extension

The Qt migration MUST reproduce all existing NoteKit user-facing features before any new
functionality is introduced. Required parity features include: tree-based note
navigation, Markdown editing with syntax highlighting, hand-drawn input
(mouse/touch/digitiser), LaTeX math rendering, and file-based note storage in the
existing on-disk format.

**Rationale**: Users of the current GTK build MUST be able to migrate without loss of
functionality. Adding new features during migration risks scope creep, integration
complexity, and delayed delivery of a usable Qt build.

### V. Minimal Dependency Footprint

Runtime dependencies beyond Qt 6.10 MUST be minimised. Each non-Qt dependency MUST be
justified in the relevant `CMakeLists.txt` comment or documented in the Technology Stack
section below. Every dependency MUST be available on Linux, Windows, and macOS, and MUST
either have active upstream maintenance or be vendored within the repository.

**Rationale**: Fewer dependencies simplify packaging, cross-platform installation, and
long-term maintenance. Qt 6 provides sufficient APIs to eliminate several current GTK
dependencies entirely.

## Technology Stack

- **UI Toolkit**: Qt 6.10 — Qt Widgets (primary; Qt Quick permitted for specific
  components where QML is clearly advantageous)
- **Language**: C++17 minimum; C++23 permitted where supported by all target compilers
  (MSVC, GCC, Clang)
- **Build System**: CMake ≥ 3.21 (required for first-class Qt 6 CMake integration)
- **IDE / Project File**: Qt Creator (CMakeLists.txt-based project)
- **Syntax Highlighting**: QSyntaxHighlighter (replacing GtkSourceView/GtkSourceViewmm)
- **Note Storage**: JSON config via jsoncpp (retained); Markdown files as plain UTF-8
- **LaTeX Math**: cLaTeXMath (retained as vendored/submodule dependency)
- **Compression**: zlib (retained; Qt provides access via QByteArray::qCompress as
  alternative — evaluate during plan phase)
- **Fonts**: fontconfig on Linux; Qt font APIs on Windows/macOS (platform-conditional)
- **Target Platforms**: Linux x86_64, Windows x86_64, macOS arm64 + x86_64

## Migration Standards

- GTK and gtkmm headers MUST NOT appear in any file after its migration task is marked
  complete. A file is considered migrated only when it builds cleanly with no GTK
  includes and all functionality is exercised by the Qt equivalent.
- Source modules (navigation, drawing, notebook, settings, imagewidgets, findinfiles,
  about, mainwindow) MUST be migrated independently in an order that maintains a
  buildable and runnable application state throughout the migration.
- All Qt signal/slot connections MUST use new-style functor syntax:
  `connect(sender, &Sender::signal, receiver, &Receiver::slot)`.
- All bundled assets (icons, stylesheets, syntax language files) MUST be managed via Qt
  resource files (`.qrc`), replacing the current runtime `data/` directory lookup.
- The existing note file format (Markdown text interleaved with drawing data) MUST remain
  byte-for-byte compatible so existing user note collections open without conversion.

## Governance

This constitution supersedes all other NoteKit development practices. Amendments require:

1. A written rationale explaining why the principle change is necessary.
2. A version bump following the semantic versioning policy below.
3. An updated Sync Impact Report prepended as an HTML comment to this file.
4. A migration plan for any in-flight feature specs or open PRs affected by the change.

All feature plans (`plan.md`) MUST include a Constitution Check section citing applicable
principles. Complexity that would violate a principle MUST be justified with a Complexity
Tracking entry in the relevant `plan.md` before proceeding.

**Versioning policy**:
- MAJOR — removal or backward-incompatible redefinition of an existing principle.
- MINOR — new principle or section added, or materially expanded guidance.
- PATCH — clarification, wording, or typo fix with no semantic change.

**Compliance review**: Verified at each feature's plan-phase gate and at pull request
review time. Any reviewer MAY block a PR citing a constitution violation; the author MUST
either address the violation or justify it with a Complexity Tracking entry.

**Version**: 1.0.0 | **Ratified**: 2026-03-02 | **Last Amended**: 2026-03-02
