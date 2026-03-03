# Contract: Note File Format

**Branch**: `001-gtk-to-qt` | **Date**: 2026-03-02
**Stability**: FROZEN — this format must remain byte-for-byte compatible between the GTK3 and Qt builds.

---

## Overview

Each NoteKit note is a single UTF-8 text file with a `.md` extension. The file contains Markdown text interleaved with special drawing marker blocks that embed serialised drawing data. The format is designed to be readable as plain Markdown when opened in any text editor — drawing blocks appear as structured comments.

---

## Text File Structure

```
<markdown text>
[optional: <drawing block>]
<markdown text>
[optional: <drawing block>]
...
```

A drawing block is delimited by an opening marker line and a closing marker line. Any number of drawing blocks may appear in a file, at any position.

---

## Drawing Block Format

### Opening Marker

```
<drawing data-for="DRAWING_ID">
```

- `DRAWING_ID`: A unique identifier string for this drawing within the note file. Opaque to the user. Format: UUID or incrementing integer (historically `0`, `1`, etc.).
- The `data-for` attribute value is used as the key in the `Note::drawings` map (see data-model.md).

### Drawing Data Lines

Between the opening and closing markers, each line is a chunk of the drawing payload. The payload is:

1. **JSON object** describing all strokes, serialised to a compact string.
2. **zlib-compressed** using the default zlib deflate stream (no Qt header).
3. **Base64-encoded** (standard alphabet, no line breaks within a chunk; split across lines at 76 characters for readability).

Each Base64 line appears as a plain text line between the markers.

### Closing Marker

```
</drawing>
```

### Full Example

```markdown
Here is some text.

<drawing data-for="0">
eJyLjk8tLk5MT1UoS8wpTgUAFNsEBQ==
</drawing>

More text follows here.
```

---

## Drawing JSON Schema

The JSON payload (before compression) conforms to the following structure:

```json
{
  "version": 1,
  "width": <integer: canvas width in pixels>,
  "height": <integer: canvas height in pixels>,
  "strokes": [
    {
      "color": "<#RRGGBB hex string>",
      "width": <float: base stroke width in pixels>,
      "points": [
        [<float: x>, <float: y>, <float: pressure 0.0–1.0>],
        ...
      ]
    },
    ...
  ]
}
```

**Invariants**:
- `version` is always `1` for the current format. Future incompatible changes increment this value.
- `width` and `height` are positive integers.
- Each point is a 3-element array: `[x, y, pressure]`.
- `color` is always a 7-character string `#` followed by 6 uppercase hex digits.
- `width` (stroke base width) is a positive float.
- `strokes` may be an empty array for a blank drawing.

---

## Compatibility Requirements

- The Qt build MUST produce files where drawing blocks are byte-identical to those produced by the GTK3 build for the same stroke data.
- The Qt build MUST correctly parse all files produced by the GTK3 build, including files with zero drawing blocks, files with multiple drawing blocks, and files where drawing blocks appear between markdown headings or list items.
- The Qt build MUST NOT modify the text content of a note except when the user explicitly edits it.
- On open: drawing blocks are parsed and removed from the `QTextDocument`; they are re-inserted on save.
- On save: drawing blocks are re-serialised in the same positions as when the file was opened.

---

## Error Handling

| Condition | Behaviour |
|-----------|-----------|
| Malformed drawing marker (no `data-for`) | Skip block; display raw text; log warning |
| Base64 decode failure | Skip block; display raw text; log warning |
| zlib decompress failure | Skip block; display raw text; log warning |
| JSON parse failure | Skip block; display raw text; log warning |
| Unknown `version` value | Skip block; display raw text; log warning |
| Drawing block with no matching closing marker | Treat remaining file as drawing data up to EOF; log warning |
