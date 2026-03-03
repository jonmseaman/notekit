# Contract: Application Settings Schema

**Branch**: `001-gtk-to-qt` | **Date**: 2026-03-02
**Stability**: STABLE — keys are additive-compatible; existing keys must not be renamed or removed.

---

## Overview

NoteKit stores user preferences using `QSettings`. On first launch of the Qt build, existing values from the legacy JSON config (`default_config.json` / user's JSON override) are migrated once and the originals are left untouched for GTK3 backwards compatibility.

---

## Settings Keys

All keys are written in the `NoteKit` application group.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `base-path` | `QString` | `$HOME/Notes` | Absolute path to the root of the user's note collection. |
| `active-document` | `QString` | `""` | Absolute path of the note open when the application last closed. Restored on startup if the file still exists. |
| `sidebar/visible` | `bool` | `true` | Whether the navigation sidebar panel is shown. |
| `presentation-mode` | `bool` | `false` | Whether the application is in fullscreen/presentation mode. Restored on startup. |
| `syntax-highlighting` | `bool` | `true` | Whether markdown syntax highlighting is enabled in the editor. |
| `window/geometry` | `QByteArray` | (empty) | Serialised window geometry (`QMainWindow::saveGeometry()`). |
| `window/state` | `QByteArray` | (empty) | Serialised dock/splitter/toolbar state (`QMainWindow::saveState()`). |
| `palette/count` | `int` | `1` | Number of colour palettes defined. |
| `palette/N/name` | `QString` | `"Default"` | Name of palette N (N = 0-based index). |
| `palette/N/colors` | `QStringList` | 8 `#RRGGBB` strings | Colour list for palette N, each entry a `#RRGGBB` hex string. |
| `palette/N/active` | `int` | `0` | Index of the currently selected colour within palette N. |
| `active-palette` | `int` | `0` | Index of the currently active palette. |

---

## Migration from Legacy JSON Config

On first launch of the Qt build, the application checks for the legacy JSON config at:
- Linux: `$XDG_CONFIG_HOME/notekit/config.json` (fallback: `~/.config/notekit/config.json`)
- Windows: `%APPDATA%\notekit\config.json`
- macOS: `~/Library/Application Support/notekit/config.json`

**Mapping from legacy JSON to QSettings**:

| Legacy JSON key | QSettings key |
|-----------------|---------------|
| `base-path` | `base-path` |
| `active-document` | `active-document` |
| `colors` (array of hex strings) | `palette/0/colors` |
| `csd` | (dropped — not applicable to Qt) |
| `sidebar` | `sidebar/visible` |
| `presentation-mode` | `presentation-mode` |
| `syntax-highlighting` | `syntax-highlighting` |

After migration, the legacy JSON file is **not modified or deleted**. The Qt build marks migration complete by writing `migration/legacy-json-imported = true` in QSettings.

---

## Invariants

- `palette/N/colors` must contain at least 1 colour.
- `palette/N/active` must be in range [0, `palette/N/colors.count()` - 1].
- `active-palette` must be in range [0, `palette/count` - 1].
- `base-path` must be a non-empty string. If the path does not exist at startup, the user is prompted to choose a new directory.

---

## Backwards Compatibility

- Settings keys listed in this contract are never renamed or removed; only new keys are added in future versions.
- Unrecognised keys written by future versions are silently ignored when read by this version.
- The legacy JSON config is never modified by the Qt build to preserve GTK3 backwards compatibility during the transition period.
