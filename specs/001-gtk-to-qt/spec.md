# Feature Specification: GTK3 to Qt Migration

**Feature Branch**: `001-gtk-to-qt`
**Created**: 2026-03-02
**Status**: Draft
**Input**: User description: "Migrate NoteKit from GTK3 to QT"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Full Note Editing Experience (Priority: P1)

A user opens the Qt-based NoteKit application and creates, edits, and reads their existing markdown notes — with inline formatting rendered as they type — exactly as they could in the GTK3 version. No migration steps, no data conversion, no loss of content.

**Why this priority**: The core value of NoteKit is editing and reading notes. If existing notes cannot be opened and edited with full fidelity, the migration delivers no user value.

**Independent Test**: Can be fully tested by launching the Qt build, opening an existing note directory, and verifying all text content, formatting, and LaTeX math renders correctly.

**Acceptance Scenarios**:

1. **Given** a user has a directory of existing NoteKit notes, **When** they open the Qt-based application pointing at that directory, **Then** all notes appear in the navigation tree and open with all text, formatting, and math intact.
2. **Given** a user is editing a note, **When** they type markdown syntax, **Then** it renders inline (e.g., bold, italic, code, headings, LaTeX math) with the same visual behavior as the GTK3 version.
3. **Given** a user has an existing note with drawings embedded, **When** they open that note, **Then** all drawings are visible and correct.

---

### User Story 2 - Drawing and Pen Annotation (Priority: P3)

A user annotates a note using their mouse, touchscreen, or stylus. They draw freehand strokes with pressure sensitivity, switch colors, and use eraser mode — with all annotations saved alongside the note text.

**Why this priority**: Hand-drawn annotations are a key differentiator for NoteKit over plain text editors. Loss of this capability would eliminate a primary use case.

**Independent Test**: Can be fully tested by opening a note, drawing several strokes with different colors and pressure, saving, closing, and reopening — verifying all strokes appear exactly as drawn.

**Acceptance Scenarios**:

1. **Given** a user is viewing a note, **When** they switch to drawing mode and draw strokes, **Then** strokes appear immediately on screen with correct color and width.
2. **Given** a user draws with a pressure-sensitive stylus, **When** strokes are rendered, **Then** stroke width varies according to applied pressure.
3. **Given** a user uses eraser mode, **When** they drag over existing strokes, **Then** those strokes are removed.
4. **Given** a user has drawn annotations, **When** they save and reopen the note, **Then** all drawings are preserved exactly.

---

### User Story 3 - Note Navigation and Organization (Priority: P2)

A user navigates their hierarchical folder and note structure in the sidebar, creates new notes and folders, renames items, and moves notes between folders using drag-and-drop.

**Why this priority**: Organization is the structural backbone of the application. Without functional navigation, users cannot manage more than a handful of notes.

**Independent Test**: Can be fully tested by performing create, rename, move, and delete operations on notes and folders in the navigation sidebar, verifying the file system reflects all changes.

**Acceptance Scenarios**:

1. **Given** a user has a nested folder structure, **When** they open the application, **Then** the sidebar displays the full hierarchy with expand/collapse controls.
2. **Given** a user right-clicks a folder, **When** they select "New Note" or "New Folder", **Then** the item is created and appears in the correct location.
3. **Given** a user drags a note to a different folder, **When** the drop completes, **Then** the note moves to the new location and the file system reflects the change.
4. **Given** a user double-clicks a note name, **When** they type a new name and confirm, **Then** the note is renamed on disk.

---

### User Story 4 - Find in Files (Priority: P4)

A user searches for text across all their notes and navigates directly to matching notes from the results list.

**Why this priority**: Cross-note search is essential for retrieving information in large note collections. It can be delivered as a standalone feature.

**Independent Test**: Can be fully tested by running a search for a known term across a note directory and verifying the correct notes appear in results, navigating to one, and confirming the term is visible.

**Acceptance Scenarios**:

1. **Given** a user opens the find-in-files panel, **When** they enter a search term, **Then** a list of matching notes appears with context snippets.
2. **Given** search results are shown, **When** the user clicks a result, **Then** the corresponding note opens and the matching text is visible.

---

### User Story 5 - Settings and Preferences (Priority: P5)

A user's existing settings and color palette preferences carry over to the Qt version without manual reconfiguration.

**Why this priority**: Settings continuity reduces friction during transition. Can be deferred if needed since manual reconfiguration is a one-time inconvenience.

**Independent Test**: Can be fully tested by copying the existing settings file, launching the Qt version, and verifying all preferences (theme, color palettes, window layout) are respected.

**Acceptance Scenarios**:

1. **Given** a user has configured custom color palettes in the GTK3 version, **When** they launch the Qt version, **Then** their palettes are available without manual re-entry.
2. **Given** a user has saved window layout preferences, **When** the Qt version starts, **Then** the window opens in the same layout state.

---

### Edge Cases

- What happens when a note file contains content from a future or unknown format version?
- How does the application behave when the notes directory is unavailable or read-only?
- What happens when a drawing file is corrupted or partially written?
- How does the application handle very large note directories (thousands of notes) during startup?
- What happens when a user's stylus driver reports pressure values outside expected range?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The application MUST open existing NoteKit note directories without any data conversion or migration step.
- **FR-002**: The application MUST render and edit markdown-formatted text with inline proximity rendering (markup hides near the cursor, shows at a distance).
- **FR-003**: The application MUST render LaTeX math expressions inline within notes.
- **FR-004**: The application MUST support freehand drawing with pressure-sensitive strokes in all notes.
- **FR-005**: The application MUST support eraser mode to remove drawn strokes.
- **FR-006**: The application MUST display the hierarchical note tree in a sidebar with expand/collapse navigation.
- **FR-007**: Users MUST be able to create, rename, move (via drag-and-drop), and delete notes and folders from the sidebar.
- **FR-008**: The application MUST support find-in-files search across all notes in the current directory.
- **FR-009**: The application MUST support find-and-replace within the currently open note.
- **FR-010**: The application MUST preserve all existing keyboard shortcuts for editing, navigation, drawing mode switching, and search.
- **FR-011**: The application MUST build and produce a runnable binary on Linux, Windows, and macOS.
- **FR-012**: User settings and color palette configurations MUST be read from the existing settings storage location without requiring manual migration.
- **FR-013**: The application MUST save notes and drawings to the same on-disk format used by the GTK3 version, ensuring forward and backward file compatibility.

### Key Entities

- **Note**: A markdown text document with optional embedded drawing data, stored as a file on disk.
- **Drawing**: A collection of freehand strokes associated with a note, serialized alongside the note file.
- **Stroke**: A single freehand pen mark defined by a sequence of coordinates, pressure values, and a color.
- **Folder**: A directory on disk that organizes notes hierarchically.
- **Color Palette**: A named set of drawing colors configured per-user and persisted in settings.
- **Settings Profile**: All user preferences including palettes, window state, and behavior options.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of notes created with the GTK3 version open and display correctly in the Qt version without any manual conversion or data loss.
- **SC-002**: All 13 functional requirements are verifiably met — each feature accessible and functioning as described in acceptance scenarios.
- **SC-003**: The application builds successfully and produces runnable binaries on all three target platforms: Linux, Windows, and macOS.
- **SC-004**: Users transitioning from the GTK3 version to the Qt version require zero manual steps to access their existing notes and settings.
- **SC-005**: Application startup time (from launch to first interactive window) is within 20% of the GTK3 version measured on equivalent hardware.
- **SC-006**: No regression in drawing responsiveness — strokes appear on screen within the same frame as stylus input under normal use.

## Assumptions

- The on-disk note format (Markdown text + JSON-serialized drawing data) will not change. File compatibility is a hard constraint.
- User settings storage paths and format are preserved or transparently migrated by the application on first launch — no user action required.
- The C++17 language standard will be retained for the implementation.
- The Meson build system will be preserved as the primary build system.
- LaTeX math rendering capability (currently provided by cLaTeXMath or equivalent) must be available in the Qt build.
- The migration is a full replacement — the GTK3 dependency is removed, not kept alongside Qt.
- Tablet/stylus pressure sensitivity is supported through whatever input APIs the target platforms provide.
