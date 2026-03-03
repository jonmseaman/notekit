#include "drawing.h"
// T036: base64.h replaced — use QByteArray::toBase64 / fromBase64

#include <QPainterPath>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QByteArray>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <zlib.h>
#include <sstream>
#include <json/json.h>

// ── CStroke ───────────────────────────────────────────────────────────────────

void CStroke::Reset()
{
    xcoords.clear();
    ycoords.clear();
    pcoords.clear();
    r = g = b = 0;
    a = 1;
}

void CStroke::Append(float x, float y, float p)
{
    xcoords.push_back(x);
    ycoords.push_back(y);
    pcoords.push_back(p);
}

void CStroke::GetHead(float &x, float &y) const
{
    if (!xcoords.empty()) {
        x = xcoords.back();
        y = ycoords.back();
    } else {
        x = y = 0;
    }
}

void CStroke::Simplify()
{
    // Trivial stub — full simplification is Phase 5
}

void CStroke::Render(QPainter &painter, float basex, float basey, int start_index) const
{
    if (xcoords.size() < 2) return;

    QColor c;
    c.setRgbF(r, g, b, a);
    painter.setRenderHint(QPainter::Antialiasing);

    for (int i = std::max(1, start_index); i < (int)xcoords.size(); ++i) {
        float pressure = pcoords[i];
        float width = 2.0f * pressure;  // base width = 2px × pressure
        QPen pen(c, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawLine(
            QPointF(basex + xcoords[i-1], basey + ycoords[i-1]),
            QPointF(basex + xcoords[i],   basey + ycoords[i]));
    }
}

void CStroke::GetBBox(float &x0, float &x1, float &y0, float &y1, int start_index) const
{
    x0 = y0 =  1e9f;
    x1 = y1 = -1e9f;
    for (int i = start_index; i < (int)xcoords.size(); ++i) {
        x0 = std::min(x0, xcoords[i]);
        x1 = std::max(x1, xcoords[i]);
        y0 = std::min(y0, ycoords[i]);
        y1 = std::max(y1, ycoords[i]);
    }
}

void CStroke::ForceMinXY(float x, float y)
{
    for (auto &v : xcoords) v -= x;
    for (auto &v : ycoords) v -= y;
}

// ── CBoundDrawing ─────────────────────────────────────────────────────────────

CBoundDrawing::CBoundDrawing(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TabletTracking);
    setAttribute(Qt::WA_NoSystemBackground);   // don't erase background before paint
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
}

void CBoundDrawing::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (!m_cache.isNull())
        painter.drawPixmap(0, 0, m_cache);

    // Render in-progress stroke for low-latency preview (T025)
    if (m_drawing && m_currentStroke.xcoords.size() >= 2)
        m_currentStroke.Render(painter, 0, 0);
}

void CBoundDrawing::Redraw()
{
    m_cache = QPixmap(std::max(1, w), std::max(1, h));
    m_cache.fill(Qt::transparent);
    QPainter p(&m_cache);
    for (const auto &s : strokes)
        s.Render(p, 0, 0);
    update();
}

bool CBoundDrawing::UpdateSize(int neww, int newh, int dx, int dy)
{
    neww -= dx; newh -= dy;
    if (neww < 1 || newh < 1) return false;
    if (dx != 0 || dy != 0) {
        for (auto &s : strokes) {
            for (auto &v : s.xcoords) v -= dx;
            for (auto &v : s.ycoords) v -= dy;
        }
        RebuildStrokefinder();
    }
    w = neww; h = newh;
    setFixedSize(w, h);
    Redraw();
    return true;
}

void CBoundDrawing::RecalculateSize()
{
    int neww = 1, newh = 1;
    for (const auto &s : strokes) {
        for (int i = 0; i < (int)s.xcoords.size(); ++i) {
            neww = std::max(neww, (int)s.xcoords[i] + 1);
            newh = std::max(newh, (int)s.ycoords[i] + 1);
        }
    }
    if (neww != w || newh != h) {
        w = neww; h = newh;
        setFixedSize(w, h);
    }
}

void CBoundDrawing::RebuildStrokefinder()
{
    strokefinder.clear();
    for (int i = 0; i < (int)strokes.size(); ++i) {
        const auto &s = strokes[i];
        for (int j = 0; j < (int)s.xcoords.size(); ++j) {
            int bk = BUCKET((int)s.xcoords[j], (int)s.ycoords[j]);
            strokefinder.insert({bk, {i, j}});
        }
    }
}

bool CBoundDrawing::AddStroke(CStroke &s, float dx, float dy, bool force)
{
    // Offset stroke coordinates
    for (auto &v : s.xcoords) v += dx;
    for (auto &v : s.ycoords) v += dy;
    strokes.push_back(s);
    RebuildStrokefinder();
    RecalculateSize();
    Redraw();
    emit strokeAdded();
    return true;
}

void CBoundDrawing::EraseAt(float x, float y, float radius, bool whole_stroke)
{
    int bk = BUCKET((int)x, (int)y);
    float r2 = radius * radius;

    std::vector<int> toRemove;
    for (auto it = strokefinder.begin(); it != strokefinder.end(); ++it) {
        int idx = it->second.index;
        int off = it->second.offset;
        if (idx >= (int)strokes.size()) continue;
        float dx = strokes[idx].xcoords[off] - x;
        float dy = strokes[idx].ycoords[off] - y;
        if (dx*dx + dy*dy <= r2) {
            toRemove.push_back(idx);
        }
    }
    // Remove duplicates and erase in reverse order
    std::sort(toRemove.begin(), toRemove.end());
    toRemove.erase(std::unique(toRemove.begin(), toRemove.end()), toRemove.end());
    for (int i = (int)toRemove.size() - 1; i >= 0; --i)
        strokes.erase(strokes.begin() + toRemove[i]);

    if (!toRemove.empty()) {
        RebuildStrokefinder();
        RecalculateSize();
        Redraw();
        emit strokeErased();
    }
}

// ── Input handling (T025) ─────────────────────────────────────────────────────
// Mouse events: fixed pressure 1.0 (mouse fallback for tablet)
// Tablet events: per-point pressure from QTabletEvent::pressure()

void CBoundDrawing::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) { QWidget::mousePressEvent(event); return; }
    event->accept();

    QPointF pos = event->position();
    if (m_eraseMode) {
        EraseAt(pos.x(), pos.y(), ERASE_RADIUS, false);
        return;
    }

    m_currentStroke.Reset();
    m_currentStroke.r = (float)m_drawColor.redF();
    m_currentStroke.g = (float)m_drawColor.greenF();
    m_currentStroke.b = (float)m_drawColor.blueF();
    m_currentStroke.a = 1.0f;
    m_currentStroke.Append(pos.x(), pos.y(), 1.0f);
    m_drawing = true;
}

void CBoundDrawing::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    QPointF pos = event->position();

    if (m_eraseMode) {
        if (event->buttons() & Qt::LeftButton)
            EraseAt(pos.x(), pos.y(), ERASE_RADIUS, false);
        return;
    }

    if (!m_drawing) return;
    m_currentStroke.Append(pos.x(), pos.y(), 1.0f);
    update(); // lightweight repaint — only paints in-progress stroke
}

void CBoundDrawing::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) { QWidget::mouseReleaseEvent(event); return; }
    event->accept();

    if (!m_drawing) return;
    m_drawing = false;

    if (m_currentStroke.xcoords.size() >= 2) {
        m_currentStroke.Simplify();
        AddStroke(m_currentStroke, 0.0f, 0.0f);
    } else if (!m_currentStroke.xcoords.empty()) {
        // Single-point tap → make a visible dot by duplicating the point
        m_currentStroke.Append(m_currentStroke.xcoords.back() + 0.5f,
                                m_currentStroke.ycoords.back() + 0.5f, 0.5f);
        AddStroke(m_currentStroke, 0.0f, 0.0f);
    }
    m_currentStroke.Reset();
    update();
}

void CBoundDrawing::tabletEvent(QTabletEvent *event)
{
    event->accept();
    QPointF pos = event->position();

    if (event->type() == QEvent::TabletPress) {
        if (m_eraseMode) {
            EraseAt(pos.x(), pos.y(), ERASE_RADIUS, false);
            return;
        }
        m_currentStroke.Reset();
        m_currentStroke.r = (float)m_drawColor.redF();
        m_currentStroke.g = (float)m_drawColor.greenF();
        m_currentStroke.b = (float)m_drawColor.blueF();
        m_currentStroke.a = 1.0f;
        m_currentStroke.Append(pos.x(), pos.y(), (float)event->pressure());
        m_drawing = true;

    } else if (event->type() == QEvent::TabletMove) {
        if (m_eraseMode) {
            EraseAt(pos.x(), pos.y(), ERASE_RADIUS, false);
            return;
        }
        if (!m_drawing) return;
        m_currentStroke.Append(pos.x(), pos.y(), (float)event->pressure());
        update();

    } else if (event->type() == QEvent::TabletRelease) {
        if (!m_drawing) return;
        m_drawing = false;
        if (m_currentStroke.xcoords.size() >= 2) {
            m_currentStroke.Simplify();
            AddStroke(m_currentStroke, 0.0f, 0.0f);
        }
        m_currentStroke.Reset();
        update();
    }
}

// ── Serialization (contracts/note-format.md) ──────────────────────────────────
// zlib compressed (raw deflate, NOT QByteArray::qCompress) + base64 encoded

std::string CBoundDrawing::Serialize()
{
    // Build JSON per Drawing JSON Schema
    Json::Value root;
    root["version"] = 1;
    root["width"]   = w;
    root["height"]  = h;
    root["strokes"] = Json::Value(Json::arrayValue);

    for (const auto &s : strokes) {
        Json::Value js;
        // Color as #RRGGBB uppercase hex
        char colbuf[8];
        snprintf(colbuf, sizeof(colbuf), "#%02X%02X%02X",
                 (int)(s.r * 255), (int)(s.g * 255), (int)(s.b * 255));
        js["color"] = colbuf;
        js["width"] = 2.0;  // base width
        js["points"] = Json::Value(Json::arrayValue);
        for (int i = 0; i < (int)s.xcoords.size(); ++i) {
            Json::Value pt(Json::arrayValue);
            pt.append(s.xcoords[i]);
            pt.append(s.ycoords[i]);
            pt.append(s.pcoords[i]);
            js["points"].append(pt);
        }
        root["strokes"].append(js);
    }

    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";
    std::string json_str = Json::writeString(wb, root);

    // zlib compress (raw deflate)
    uLongf compLen = compressBound((uLong)json_str.size());
    std::vector<unsigned char> compressed(compLen);
    if (compress2(compressed.data(), &compLen,
                  (const Bytef*)json_str.data(), (uLong)json_str.size(),
                  Z_DEFAULT_COMPRESSION) != Z_OK) {
        return {};
    }
    compressed.resize(compLen);

    // T036: Base64 encode via QByteArray::toBase64 (replaces base64.h)
    // Standard alphabet (A-Za-z0-9+/ with = padding) — byte-for-byte compatible
    QByteArray rawData = QByteArray::fromRawData(
        reinterpret_cast<const char *>(compressed.data()),
        static_cast<int>(compLen));
    QByteArray b64qt = rawData.toBase64(QByteArray::Base64Encoding);
    std::string b64 = b64qt.toStdString();

    // Split into 76-char lines per the note format contract
    std::string out;
    for (size_t i = 0; i < b64.size(); i += 76) {
        out += b64.substr(i, 76);
        out += '\n';
    }
    return out;
}

void CBoundDrawing::Deserialize(const std::string &input)
{
    strokes.clear();

    // Remove line breaks to get raw base64
    std::string b64;
    for (char c : input)
        if (c != '\n' && c != '\r') b64 += c;

    if (b64.empty()) return;

    // T036: Base64 decode via QByteArray::fromBase64 (replaces base64.h)
    QByteArray b64qt = QByteArray::fromStdString(b64);
    QByteArray compressedQBA = QByteArray::fromBase64(b64qt, QByteArray::Base64Encoding);
    if (compressedQBA.isEmpty() && !b64qt.isEmpty()) return; // decode failed

    std::vector<unsigned char> compressed(
        reinterpret_cast<const unsigned char *>(compressedQBA.constData()),
        reinterpret_cast<const unsigned char *>(compressedQBA.constData()) + compressedQBA.size());

    // Decompress
    std::vector<unsigned char> uncompressed(std::max<size_t>(compressed.size() * 8, 256));
    uLongf uncompLen = (uLongf)uncompressed.size();
    int ret = Z_BUF_ERROR;
    while (ret == Z_BUF_ERROR) {
        ret = uncompress(uncompressed.data(), &uncompLen,
                         compressed.data(), (uLong)compressed.size());
        if (ret == Z_BUF_ERROR) {
            uncompressed.resize(uncompressed.size() * 2);
            uncompLen = (uLongf)uncompressed.size();
        }
    }
    if (ret != Z_OK) return;
    uncompressed.resize(uncompLen);

    std::string json_str(uncompressed.begin(), uncompressed.end());

    Json::Value root;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream is(json_str);
    if (!Json::parseFromStream(rb, is, &root, &errs)) return;

    if (!root.isMember("strokes") || !root["strokes"].isArray()) return;

    for (const auto &js : root["strokes"]) {
        CStroke s;
        // Parse color #RRGGBB
        std::string color = js.get("color", "#000000").asString();
        if (color.size() == 7 && color[0] == '#') {
            unsigned r, g, b;
            sscanf(color.c_str() + 1, "%02X%02X%02X", &r, &g, &b);
            s.r = r / 255.0f;
            s.g = g / 255.0f;
            s.b = b / 255.0f;
        }
        s.a = 1.0f;

        if (js.isMember("points") && js["points"].isArray()) {
            for (const auto &pt : js["points"]) {
                if (pt.isArray() && pt.size() >= 3) {
                    s.xcoords.push_back(pt[0].asFloat());
                    s.ycoords.push_back(pt[1].asFloat());
                    s.pcoords.push_back(pt[2].asFloat());
                }
            }
        }
        strokes.push_back(s);
    }

    w = root.get("width",  1).asInt();
    h = root.get("height", 1).asInt();
    setFixedSize(std::max(1, w), std::max(1, h));
    RebuildStrokefinder();
    Redraw();
}
