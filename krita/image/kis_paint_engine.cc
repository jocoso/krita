/*
 *  copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

    // We need to be faster: we've to create our own functions.
    // We need:
    // - drawLine
    // - drawPolygon
    // - drawArc
    // - drawEllipse
    // - drawText
    // All with filling enabled. They need to be fast: so we will not use
    // KisPainter (too slow to initialize), perhaps it's better to draw directly
    // on the Device. What about antialiasing?

#include "kis_paint_engine.h"

#include <QtCore>
#include <QtGui>

#include "kis_paint_device.h"
#include "kis_painter.h"
#include "KoCompositeOp.h"


class KisPaintEngine::KisPaintEnginePrivate {
public:
    KisPaintDevice * dev;
    KisPaintDeviceSP buffer;
    QRegion dirty;
// About the state
    QPaintEngine::DirtyFlags flags;
    QMatrix matrix;
    qreal opacity;
    QPainter::RenderHints renderHints;
    QPen pen;
    QBrush brush, backgroundBrush;
    Qt::BGMode backgroundMode;
    QPainter::CompositionMode compositionMode;
    QPointF brushOrigin;
    QPainterPath clipPath;
    QFont font;
//     QPicture img;
    bool isClipEnabled;
};


KisPaintEngine::KisPaintEngine()
{
    d = new KisPaintEnginePrivate;

    // Set capabilities
    gccaps = AllFeatures;
}

KisPaintEngine::~KisPaintEngine()
{
    delete d;
}

bool KisPaintEngine::begin(QPaintDevice *pdev)
{
    KisPaintDevice * dev = dynamic_cast<KisPaintDevice*>( pdev );
    Q_ASSERT_X(dev, "KisPaintEngine::begin",
               "QPaintEngine can only work on KisPaintDevices, nothing else!");
    d->dev = dev;
    d->flags = 0;
    d->matrix.reset();
    d->buffer = new KisPaintDevice(dev->colorSpace());
    d->dirty = QRegion();

    // XXX: Start transaction for undo?
    return true;
}

bool KisPaintEngine::end()
{
    // XXX: End transaction for undo?
    if (d->dirty.boundingRect().isNull())
        return true;

    QRect r = d->dirty.boundingRect();

    KisPainter kp(d->dev);
    kp.bitBlt(r.x(), r.y(), d->dev->colorSpace()->compositeOp( COMPOSITE_COPY ),
              d->buffer, OPACITY_OPAQUE, 0, 0, r.width(), r.height());
    kp.end();

    d->dev->setDirty( r );
    return true;
}

void KisPaintEngine::updateState(const QPaintEngineState &state)
{
    QPaintEngine::DirtyFlags flags = state.state();
    d->flags |= state.state();

    if (flags & DirtyOpacity) {
        d->opacity = state.opacity();
        if (d->opacity > 1)
            d->opacity = 1;
        if (d->opacity < 0)
            d->opacity = 0;
        // Force update pen/brush as to get proper alpha colors propagated
        flags |= DirtyPen;
        flags |= DirtyBrush;
    }

    if (flags & DirtyTransform) {
        d->matrix = state.matrix();
    }
    if (flags & DirtyPen) {
        d->pen = state.pen();
    }
    if (flags & (DirtyBrush | DirtyBrushOrigin)) {
        d->brush = state.brush();
        d->brushOrigin = state.brushOrigin();
    }
    if (flags & DirtyFont) {
        d->font = state.font();
    }
    if (flags & DirtyBackground) {
        d->backgroundBrush = state.backgroundBrush();
    }
    if (flags & DirtyBackgroundMode) {
        d->backgroundMode = state.backgroundMode();
    }
    if (flags & DirtyCompositionMode) {
        d->compositionMode = state.compositionMode();
    }
    if (flags & DirtyClipEnabled) {
        d->isClipEnabled = state.isClipEnabled();
    }
    if (flags & DirtyClipRegion) {
        QPainterPath clipPath;
        clipPath.addRect(state.clipRegion().boundingRect());
        d->clipPath = d->matrix.map(clipPath);
    }
    if (flags & DirtyClipPath) {
        d->clipPath = d->matrix.map(state.clipPath());
    }
    if (flags & DirtyHints) {
        d->renderHints = state.renderHints();
    }
}

void KisPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    QPaintEngine::drawRects( rects, rectCount );
}

void KisPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    QPaintEngine::drawRects( rects, rectCount );
}


void KisPaintEngine::drawLines(const QLine *lines, int lineCount)
{
    QPaintEngine::drawLines( lines, lineCount );
}

void KisPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    QPaintEngine::drawLines( lines, lineCount );
}


void KisPaintEngine::drawEllipse(const QRectF &r)
{
    QPaintEngine::drawEllipse( r );
}

void KisPaintEngine::drawEllipse(const QRect &r)
{
    QPaintEngine::drawEllipse( r );
}

void KisPaintEngine::initPainter(QPainter &p)
{
    if (d->flags & (DirtyClipRegion | DirtyClipPath)) {
        p.setClipPath(d->clipPath);
        d->dirty += QRegion(d->clipPath.boundingRect().toRect());
    }
    if (d->flags & DirtyTransform) {
        p.setMatrix(d->matrix*p.worldMatrix());
    }
    if (d->flags & DirtyPen) {
        p.setPen(d->pen);
    }
    if (d->flags & (DirtyBrush | DirtyBrushOrigin)) {
        p.setBrush(d->brush);
        p.setBrushOrigin(d->brushOrigin);
    }
    if (d->flags & DirtyFont) {
        p.setFont(d->font);
    }
    if (d->flags & DirtyBackground) {
        p.setBackground(d->backgroundBrush);
    }
    if (d->flags & DirtyBackgroundMode) {
        p.setBackgroundMode(d->backgroundMode);
    }
    if (d->flags & DirtyCompositionMode) {
        p.setCompositionMode(d->compositionMode);
    }
    if (d->flags & DirtyHints) {
        p.setRenderHints(d->renderHints);
    }
}

void KisPaintEngine::drawPath(const QPainterPath &path)
{
    QPainterPath newPath = d->matrix.map(path);
    QRect r = newPath.boundingRect().toRect();

    QImage img(r.width(), r.height(), QImage::Format_ARGB32);
    img.fill(0);
    QPainter p;
    p.begin(&img);
    p.translate(-r.x(), -r.y());
    initPainter(p);
    p.drawPath(path);
    p.end();

    KisPaintDeviceSP dev = new KisPaintDevice(d->dev->colorSpace());
    dev->convertFromQImage(img, "");
    KisPainter kp(d->buffer.data());
    kp.bitBlt(r.x(), r.y(), d->dev->colorSpace()->compositeOp( COMPOSITE_OVER ),
              dev, OPACITY_OPAQUE, 0, 0, r.width(), r.height());
    kp.end();

    d->dirty += QRegion(r);
}


void KisPaintEngine::drawPoints(const QPointF *points, int pointCount)
{
    QPaintEngine::drawPoints( points, pointCount );
}

void KisPaintEngine::drawPoints(const QPoint *points, int pointCount)
{
    QPaintEngine::drawPoints( points, pointCount );
}


void KisPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_UNUSED( mode );

    QPolygonF path;
    const QPointF *point = points;

    for (int i = 0; i < pointCount; i++)
        path.append(*(point++));

    QPolygonF newPath = d->matrix.map(path);
    QRect r = newPath.boundingRect().toRect();

    QImage img(r.width(), r.height(), QImage::Format_ARGB32);
    img.fill(0);
    QPainter p;
    p.begin(&img);
    p.translate(-r.x(), -r.y());
    initPainter(p);
    p.drawPolygon(path);
    p.end();

    KisPaintDeviceSP dev = new KisPaintDevice(d->dev->colorSpace());
    dev->convertFromQImage(img, "");
    KisPainter kp(d->buffer.data());
    kp.bitBlt(r.x(), r.y(), d->dev->colorSpace()->compositeOp( COMPOSITE_OVER ),
              dev, OPACITY_OPAQUE, 0, 0, r.width(), r.height());
    kp.end();

    d->dirty += QRegion(r);
}

void KisPaintEngine::drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
{
    Q_UNUSED( mode );

    QPolygon path;
    const QPoint *point = points;

    for (int i = 0; i < pointCount; i++)
        path.append(*(point++));

    QPolygon newPath = d->matrix.map(path);
    QRect r = newPath.boundingRect();

    QImage img(r.width(), r.height(), QImage::Format_ARGB32);
    img.fill(0);
    QPainter p;
    p.begin(&img);
    p.translate(-r.x(), -r.y());
    initPainter(p);
    p.drawPolygon(path);
    p.end();

    KisPaintDeviceSP dev = new KisPaintDevice(d->dev->colorSpace());
    dev->convertFromQImage(img, "");
    KisPainter kp(d->buffer.data());
    kp.bitBlt(r.x(), r.y(), d->dev->colorSpace()->compositeOp( COMPOSITE_OVER ),
              dev, OPACITY_OPAQUE, 0, 0, r.width(), r.height());
    kp.end();

    d->dirty += QRegion(r);
}


void KisPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    drawImage( r, pm.toImage(), sr );
}

void KisPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    QPaintEngine::drawTextItem( p, textItem );
}

void KisPaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s)
{
    // XXX: Reimplement this, the default will convert the pixmap time
    // and again to a QImage
    QPaintEngine::drawTiledPixmap( r, pixmap, s );
}

void KisPaintEngine::drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                               Qt::ImageConversionFlags flags)
{
    Q_UNUSED( flags );
    // XXX: How about sub-pixel bitBlting?
    QRect srcRect = sr.toRect();
    QRect dstRect = r.toRect();

    KisPainter p( d->dev );
    // XXX: Get the right porter-duff composite op from the state, for
    // now use OVER.


#if 0
    p.bitBlt(dstRect.x(), dstRect.y(), d->dev->colorSpace()->compositeOp( COMPOSITE_OVER ),
             &pm, static_cast<quint8>( d->flags.opacity() * 255 ),
             srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height());
#else
    KisPaintDeviceSP dev = new KisPaintDevice( d->dev->colorSpace() );
    dev->convertFromQImage(pm, "");
    p.bitBlt(dstRect.x(), dstRect.y(), d->dev->colorSpace()->compositeOp( COMPOSITE_OVER ),
             dev, OPACITY_OPAQUE,
             srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height());
#endif

}
