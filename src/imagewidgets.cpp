#include "imagewidgets.h"
#include <QTextDocument>
#include <QTextFormat>
#include <QDebug>

#ifdef HAVE_CLATEXMATH
#define __OS_Linux__
#include "latex.h"
#endif

// ── CImageWidget ──────────────────────────────────────────────────────────────

CImageWidget::CImageWidget(QObject *parent)
    : QObject(parent)
{}

QSizeF CImageWidget::intrinsicSize(QTextDocument * /*doc*/, int /*posInDocument*/,
                                    const QTextFormat &format)
{
    QVariant v = format.property(ImagePixmapProp);
    if (!v.isValid()) return QSizeF(1, 1);
    QPixmap pm = qvariant_cast<QPixmap>(v);
    return pm.size();
}

void CImageWidget::drawObject(QPainter *painter, const QRectF &rect,
                               QTextDocument * /*doc*/, int /*posInDocument*/,
                               const QTextFormat &format)
{
    QVariant v = format.property(ImagePixmapProp);
    if (!v.isValid()) return;
    QPixmap pm = qvariant_cast<QPixmap>(v);
    painter->drawPixmap(rect.toRect(), pm);
}

QTextCharFormat CImageWidget::makeFormat(const QPixmap &pixmap, int baseline)
{
    QTextCharFormat fmt;
    fmt.setObjectType(ImageObjectType);
    fmt.setProperty(ImagePixmapProp, pixmap);
    fmt.setProperty(ImageBaselineProp, baseline);
    return fmt;
}

// ── CLatexWidget ──────────────────────────────────────────────────────────────

#ifdef HAVE_CLATEXMATH
CLatexWidget::CLatexWidget(const QString &source, const QColor &fgColor, QObject *parent)
    : CImageWidget(parent)
{
    render(source, fgColor);
}

void CLatexWidget::render(const QString &source, const QColor &fgColor)
{
    using namespace tex;
    unsigned int clr = 0xFF000000u
                       | (unsigned(fgColor.red())   << 16)
                       | (unsigned(fgColor.green())  << 8)
                       |  unsigned(fgColor.blue());
    try {
        TeXRender *r = LaTeX::parse(
            utf82wide(source.toStdString().c_str()),
            480, 36, 36 * 0.8f, clr);
        if (!r) return;

        int rw = r->getWidth(), rh = r->getHeight();
        m_baseline = (int)(rh * (1.0f - r->getBaseline()));
        delete r;
        // Full rendering would store pixmap here — stub for now
    } catch (...) {
        qWarning() << "CLatexWidget: render failed for" << source;
    }
}
#endif
