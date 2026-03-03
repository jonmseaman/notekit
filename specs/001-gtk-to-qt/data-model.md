# Data Model: GTK3 to Qt Migration

**Branch**: `001-gtk-to-qt` | **Date**: 2026-03-02

This document describes the core entities, their fields, relationships, and state transitions as they exist in the migrated Qt application. All entities map 1:1 to the existing GTK3 implementation — no new entities are introduced.

---

## Entities

### CStroke

A single freehand pen stroke drawn by the user.

| Field | Type | Description |
|-------|------|-------------|
| `points` | `std::vector<QPointF>` | Ordered sequence of (x, y) coordinates in drawing-area local space |
| `pressures` | `std::vector<float>` | Per-point pressure value in [0.0, 1.0]; parallel to `points` |
| `color` | `QColor` | Stroke colour |
| `width` | `float` | Base stroke width before pressure scaling (pixels) |
| `id` | `uint32_t` | Unique identifier within a drawing for selection and erasing |

**Validation rules**:
- `points.size()` == `pressures.size()` at all times
- `width` > 0
- Pressure values clamped to [0.0, 1.0]

**Serialisation**: Serialised as a JSON object in the drawing data block embedded in the note file (see contracts/note-format.md).

---

### CBoundDrawing

A drawing canvas attached to a specific position in a note. Contains zero or more strokes. Backed by a `QPixmap` cache for fast repaints.

| Field | Type | Description |
|-------|------|-------------|
| `strokes` | `std::vector<CStroke>` | All strokes in this drawing in creation order |
| `bounds` | `QRect` | Width × height of the canvas in pixels |
| `pixmap_cache` | `QPixmap` | Rendered cache; rebuilt when `strokes` changes |
| `bucket_index` | `std::map<int, std::vector<int>>` | Spatial index: bucket → stroke indices for efficient eraser hit-testing |
| `dirty` | `bool` | True when `pixmap_cache` does not reflect current `strokes` |

**State Transitions**:
```
[Idle] --stroke added--> [Dirty] --repaint--> [Idle]
[Idle] --stroke erased--> [Dirty] --repaint--> [Idle]
[Idle] --resize--> [Dirty] --repaint--> [Idle]
```

**Relationships**: One `CBoundDrawing` is embedded per drawing block in a note. A note may contain zero or more `CBoundDrawing` instances.

---

### Note

A single note document. Corresponds to one file on disk.

| Field | Type | Description |
|-------|------|-------------|
| `path` | `QString` | Absolute filesystem path to the `.md` file |
| `title` | `QString` | Display name (filename without extension) |
| `text_content` | `QString` | Full raw text of the note including drawing markers |
| `drawings` | `QMap<QString, CBoundDrawing>` | Map from drawing marker token to drawing data |
| `modified` | `bool` | True when in-memory content differs from on-disk content |

**Validation rules**:
- `path` must reference a writable file location
- Drawing markers in `text_content` must have corresponding entries in `drawings`

---

### NavigationNode

Represents one entry (file or folder) in the sidebar note tree.

| Field | Type | Description |
|-------|------|-------------|
| `display_name` | `QString` | User-visible label shown in the tree |
| `full_path` | `QString` | Absolute filesystem path |
| `node_type` | `enum { File, Folder }` | Whether this node is a leaf note or a container |
| `sort_order` | `int` | User-assigned ordering index within the parent folder |
| `children` | `QList<NavigationNode*>` | Child nodes (empty for `File` nodes) |
| `is_expanded` | `bool` | Whether the tree row is expanded (persisted in settings) |

**Relationships**: Tree structure — each `NavigationNode` has one parent (or is the root). The root corresponds to the user's notes base directory.

---

### ColorPalette

A named set of drawing colours configured by the user.

| Field | Type | Description |
|-------|------|-------------|
| `name` | `QString` | Palette name (e.g., "Default", "Ink") |
| `colors` | `QList<QColor>` | Ordered list of available colours (typically 8) |
| `active_index` | `int` | Currently selected colour index |

**Validation rules**:
- `colors` must not be empty
- `active_index` in [0, `colors.size()` - 1]

---

### AppSettings

All user-configurable application preferences. Backed by `QSettings`.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `base-path` | `QString` | `~/Notes` | Root directory of the user's note collection |
| `active-document` | `QString` | `""` | Path of the last-open note (restored on startup) |
| `colors` | `QList<QColor>` | 8 default colours | Colour palette entries |
| `sidebar` | `bool` | `true` | Whether the navigation sidebar is visible |
| `presentation-mode` | `bool` | `false` | Whether presentation (fullscreen) mode is active |
| `syntax-highlighting` | `bool` | `true` | Whether markdown syntax highlighting is enabled |
| `window-geometry` | `QByteArray` | (none) | Saved window size and position |
| `window-state` | `QByteArray` | (none) | Saved splitter and toolbar state |

---

## Relationships Summary

```
AppSettings
  └── base-path ──> NavigationNode (root)
                      └── NavigationNode (folders + notes)
                            └── Note
                                  └── CBoundDrawing (0..*)
                                        └── CStroke (0..*)

AppSettings
  └── colors ──> ColorPalette
```

---

## State: Editor Modes

The notebook editor operates in one of three mutually exclusive input modes:

```
[Text] <--toggle--> [Draw] <--toggle--> [Erase]
  |                   |                    |
  v                   v                    v
QTextEdit          CBoundDrawing       CBoundDrawing
cursor input       + QTabletEvent      + erase radius
```

| Mode | Cursor | Input Source | Active Widget |
|------|--------|-------------|---------------|
| Text | `QCursor(Qt::IBeamCursor)` | `QTextEdit` keyboard + mouse | `QTextEdit` |
| Draw | `QCursor` (pen SVG) | `QTabletEvent` / `QMouseEvent` | `CBoundDrawing` overlay |
| Erase | `QCursor` (circle) | `QTabletEvent` / `QMouseEvent` | `CBoundDrawing` overlay |
