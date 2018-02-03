
/******************************************************************************
   HotShots: Screenshot utility
   Copyright(C) 2011-2014  xbee@xbee.net

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *******************************************************************************/

#include <QtCore/QPointF>

#include <QGraphicsPathItem>
#include <QPolygonF>
#include <QPen>
#include <QBrush>

#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QColor>
#include <QGraphicsSceneMouseEvent>

#include <cmath>

#include "EditorLineArrowItem.h"
#include "HandgripItem.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// based on Kaption code: http://code.google.com/p/kaption/

EditorLineArrowItem::EditorLineArrowItem(QGraphicsItem *parent)
    : BaseItem(parent),
    m_scaleFactor(1.0),
    m_scaleMin(0.625),
    m_scaleMax(2.0),
    m_scalePivot(1.0)
{
    m_type = metaObject()->className();

    m_item = new QGraphicsPathItem(this);
    m_item->setGraphicsEffect(m_dropShadowFx);
    QBrush br(m_item->pen().color(),Qt::SolidPattern);
    m_item->setBrush(br);

    m_handgrips << new HandgripItem(this,this,0);
    m_handgrips << new HandgripItem(this,this,1);

    updateHandgrips();

    // init with 2 dummy points
    m_line << QPointF();
    m_line << QPointF();
}

EditorLineArrowItem::~EditorLineArrowItem()
{ }

QPen EditorLineArrowItem::pen() const
{
    return m_item->pen();
}

void EditorLineArrowItem::setPen(const QPen &p)
{
    m_pen = p;
    QPen pen(p.color(), p.width(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    m_item->setPen(pen);
    QBrush br(m_item->pen().color(),Qt::SolidPattern);
    m_item->setBrush(br);
    m_scaleFactor = p.width();

    // redraw if needed
    if ( !m_line[TAIL].isNull() || !m_line[HEAD].isNull() )
        redrawArrow();
}

void EditorLineArrowItem::redrawArrow()
{
    m_line[HEAD] = m_handgrips[HEAD]->scenePos();
    m_line[TAIL] = m_handgrips[TAIL]->scenePos();
    createShape(m_line[TAIL],m_line[HEAD]);
}

void EditorLineArrowItem::mousePress ( QGraphicsSceneMouseEvent * mouseEvent )
{
    if (m_initialized) // to avoid move by post selection of item
        return;

    m_line[TAIL] = mouseEvent->scenePos();
    m_handgrips[TAIL]->setPos( mapFromScene(m_line[TAIL]) );
    m_initialized = true;
}

void EditorLineArrowItem::mouseMove ( QGraphicsSceneMouseEvent * mouseEvent )
{
    m_line[HEAD] = mouseEvent->scenePos();
    createShape(m_line[TAIL],m_line[HEAD]);

    m_handgrips[TAIL]->setPos( mapFromScene(m_line[TAIL]) );
    m_handgrips[HEAD]->setPos( mapFromScene(m_line[HEAD]) );
}

BaseItem::typModeEdition EditorLineArrowItem::mouseRelease ( QGraphicsSceneMouseEvent * mouseEvent )
{
    Q_UNUSED(mouseEvent);
    return BaseItem::EDITION_NEXTITEM;
}

void EditorLineArrowItem::updatePoint(const QPointF &pt)
{
    HandgripItem *hg = qobject_cast<HandgripItem *>( sender() );

    if (!hg)
        return;

    // to avoid scene relative move
    m_line[TAIL] = m_handgrips[TAIL]->scenePos();
    m_line[HEAD] = m_handgrips[HEAD]->scenePos();
    m_line[hg->getId()] = pt;
    setPos( QPointF(0,0) );
    createShape(m_line[TAIL],m_line[HEAD]);
    m_handgrips[TAIL]->setPos( mapFromScene(m_line[TAIL]) );
    m_handgrips[HEAD]->setPos( mapFromScene(m_line[HEAD]) );
}

void EditorLineArrowItem::endEdition()
{
    // check validity
    QPointF dist = m_line[HEAD] - m_line[TAIL];
    if (dist.manhattanLength() < 6)
        deleteLater();
}

void EditorLineArrowItem::createShape(const QPointF &start, const QPointF &end)
{
    setArrow( createArrow(start, end, m_scaleFactor), end, calculateAngle(start, end) );
}

void EditorLineArrowItem::setArrow(const QPainterPath &arrow, const QPointF &p, qreal angle)
{
    m_item->setPath(arrow);
    setPos(p);
    setRotation(angle);
}

QPainterPath EditorLineArrowItem::createArrow(const QPointF &tail, const QPointF &head, qreal scaleFactor ) const
{
    QPointF p = head - tail;
    qreal length = sqrt( pow(p.x(), 2) + pow(p.y(), 2) );

    QPainterPath path;
    QPointF px0(0, 0);
    QPointF px1(-15, 7);
    QPointF px2(-15, -7);
    QPointF px3(-length, 0);

    path.lineTo(px0);
    path.lineTo(px1);
    path.lineTo(px0);
    path.lineTo(px2);
    path.lineTo(px0);
    path.lineTo(px3);

    return path;
}

qreal EditorLineArrowItem::calculateAngle(const QPointF &start, const QPointF &end) const
{
    QPointF p = end - start;

    qreal a = p.x();
    qreal c = sqrt( pow(p.x(), 2) + pow(p.y(), 2) );

    if (c == 0)
    {
        return 0;
    }

    qreal angle = acos(a / c) * 180 / M_PI;
    if ( end.y() > start.y() )
    {
        angle = 360 - angle;
    }
    return -angle;
}

qreal EditorLineArrowItem::convert(qreal o_value, qreal o_min, qreal o_max, qreal n_min, qreal n_max) const
{
    return ( (o_value - o_min) * (n_max - n_min) ) / (o_max - o_min) + n_min;
}

qreal EditorLineArrowItem::convert(qreal o_value,
                               qreal o_min,
                               qreal o_max,
                               qreal o_pivot,
                               qreal n_min,
                               qreal n_max,
                               qreal n_pivot) const
{
    if (o_min == n_min && o_max == n_max && o_pivot == n_pivot)
    {
        return o_value;
    }

    qreal n_value;
    if (o_value >= o_pivot)
    {
        n_value = convert(o_value, o_pivot, o_max, n_pivot, n_max);
    }
    else
    {
        n_value = convert(o_value, o_min, o_pivot, n_min, n_pivot);
    }
    return n_value;
}

QRectF EditorLineArrowItem::boundingRect() const
{
    return m_item->boundingRect();
}

void EditorLineArrowItem::toXml(QDomElement &elt) const
{
    elt.setAttribute( "start", IOHelper::variantToString( m_handgrips[TAIL]->scenePos() ) );
    elt.setAttribute( "end", IOHelper::variantToString( m_handgrips[HEAD]->scenePos() ) );
    elt.setAttribute( "color", IOHelper::variantToString( m_item->pen().color() ) );
    elt.setAttribute( "width", IOHelper::variantToString( m_item->pen().width() ) );
    elt.setAttribute( "shadow", IOHelper::variantToString( isShadowEnabled() ) );
}

void EditorLineArrowItem::fromXml(const QDomElement &elt)
{
    bool ok = true;
    m_initialized = true;
    m_line[TAIL] = IOHelper::stringToVariant(elt.attribute("start"),QVariant::PointF,ok).toPointF();
    m_line[HEAD] = IOHelper::stringToVariant(elt.attribute("end"),QVariant::PointF,ok).toPointF();
    setEnableShadow( IOHelper::stringToVariant(elt.attribute("shadow","true"),QVariant::Bool,ok).toBool() );

    QColor color = IOHelper::stringToVariant(elt.attribute("color"),QVariant::Color,ok).value<QColor>();
    int width = elt.attribute("width").toInt();
    m_pen = QPen(color,width);
    m_pen.setCapStyle(Qt::RoundCap);
    m_pen.setJoinStyle(Qt::RoundJoin);

    createShape(m_line[TAIL],m_line[HEAD]);

    m_handgrips[TAIL]->setPos( mapFromScene(m_line[TAIL]) );
    m_handgrips[HEAD]->setPos( mapFromScene(m_line[HEAD]) );

    setPen(m_pen);

    //setBrush(QBrush());
}

// copy constructor
BaseItem * EditorLineArrowItem::clone()
{
    EditorLineArrowItem *copiedItem = new  EditorLineArrowItem();
    copyDataTo(copiedItem);
    return copiedItem;
}
