#pragma once
// Migrated from Gtk::DrawingArea + Cairo to QWidget + QPainter (T023)
// T023: CBoundDrawing : public QWidget replaces Gtk::DrawingArea
// T025: Full mouse/tablet input handling for stroke drawing and erasing

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <vector>
#include <unordered_map>
#include <string>

// ── CStroke ──────────────────────────────────────────────────────────────────
// Pure data struct — no GTK types. Identical semantics to the original.
class CStroke {
public:
    float r = 0, g = 0, b = 0, a = 1;
    std::vector<float> xcoords;
    std::vector<float> ycoords;
    std::vector<float> pcoords;  // pressure [0.0, 1.0]

    void Reset();
    void Append(float x, float y, float p);
    void GetHead(float &x, float &y) const;
    void Simplify();
    void Render(QPainter &painter, float basex, float basey, int start_index = 1) const;
    void GetBBox(float &x0, float &x1, float &y0, float &y1, int start_index = 0) const;
    void ForceMinXY(float x, float y);
};

// ── CBoundDrawing ─────────────────────────────────────────────────────────────
// Replaces Gtk::DrawingArea — full implementation is Phase 5.
// Phase 3 only needs Serialize()/Deserialize() for note format compatibility.
class CBoundDrawing : public QWidget
{
    Q_OBJECT
public:
    explicit CBoundDrawing(QWidget *parent = nullptr);

    std::vector<CStroke> strokes;

    static const int BUCKET_SIZE = 16;
    static constexpr int BUCKET(int x, int y) {
        return ((y / BUCKET_SIZE) << 16) + (x / BUCKET_SIZE);
    }
    struct strokeRef { int index; int offset; };
    std::unordered_multimap<int, strokeRef> strokefinder;

    int w = 1, h = 1;
    bool selected = false;

    QPixmap m_cache;

    // ── Drawing state (T025) ──────────────────────────────────────────────────
    bool m_drawing   = false;    // true while a stroke is in progress
    bool m_eraseMode = false;    // true when in Erase input mode
    QColor m_drawColor = Qt::black;
    float  m_drawWidth = 2.0f;
    static constexpr float ERASE_RADIUS = 20.0f;

    CStroke m_currentStroke;     // in-progress stroke (not yet committed to strokes[])

    bool UpdateSize(int neww, int newh, int dx = 0, int dy = 0);
    void Redraw();
    void RebuildStrokefinder();
    void RecalculateSize();
    bool AddStroke(CStroke &s, float dx, float dy, bool force = false);
    void EraseAt(float x, float y, float radius, bool whole_stroke);

    // Serialise/deserialise per contracts/note-format.md
    // compress() uses raw zlib (NOT QByteArray::qCompress — incompatible header)
    // T036: base64 via QByteArray::toBase64/fromBase64 (replaces base64.h)
    std::string Serialize();
    void Deserialize(const std::string &input);

signals:
    void strokeAdded();
    void strokeErased();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void tabletEvent(QTabletEvent *event) override;
};
