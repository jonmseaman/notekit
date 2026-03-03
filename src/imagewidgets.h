#pragma once
// Migrated from Gtk::DrawingArea + Cairo to QTextObjectInterface (T014)

#include "config.h"

#include <QObject>
#include <QTextObjectInterface>
#include <QPixmap>
#include <QPainter>
#include <QSizeF>
#include <QRectF>
#include <QString>

// Custom object type IDs registered with QTextDocument layout
enum {
    ImageObjectType  = QTextFormat::UserObject + 1,
    LatexObjectType  = QTextFormat::UserObject + 2,
};

// QTextFormat property keys for embedded objects
enum {
    ImagePixmapProp = QTextFormat::UserProperty + 1,
    ImageBaselineProp = QTextFormat::UserProperty + 2,
};

// ── CImageWidget ──────────────────────────────────────────────────────────────
// Renders a QPixmap as an inline text object via QTextObjectInterface.
class CImageWidget : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    explicit CImageWidget(QObject *parent = nullptr);
    ~CImageWidget() override = default;

    // QTextObjectInterface
    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument,
                         const QTextFormat &format) override;
    void drawObject(QPainter *painter, const QRectF &rect,
                    QTextDocument *doc, int posInDocument,
                    const QTextFormat &format) override;

    // Set a pixmap and optional baseline offset
    static QTextCharFormat makeFormat(const QPixmap &pixmap, int baseline = 0);
};

#if defined(HAVE_CLATEXMATH)
// ── CLatexWidget ──────────────────────────────────────────────────────────────
// Renders LaTeX math via cLaTeXMath into a QPixmap, then delegates to CImageWidget.
class CLatexWidget : public CImageWidget
{
    Q_OBJECT
public:
    explicit CLatexWidget(const QString &source, const QColor &fgColor,
                          QObject *parent = nullptr);

    int GetBaseline() const { return m_baseline; }

private:
    int m_baseline = 0;
    void render(const QString &source, const QColor &fgColor);
};
#endif
