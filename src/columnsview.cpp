/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "columnsview.h"
#include "columnsview_p.h"

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>
#include <QPropertyAnimation>

ContentItem::ContentItem(ColumnsView *parent)
    : QQuickItem(parent),
      m_view(parent)
{
    m_slideAnim = new QPropertyAnimation(this);
    m_slideAnim->setTargetObject(this);
    m_slideAnim->setPropertyName("x");
    //TODO: from Units
    m_slideAnim->setDuration(250);
    m_slideAnim->setEasingCurve(QEasingCurve(QEasingCurve::InOutQuad));
    connect(m_slideAnim, &QPropertyAnimation::finished, this, [this] () {
        if (!m_view->currentItem()) {
            m_view->setCurrentIndex(m_items.indexOf(m_firstVisibleItem));
        }
        // If the current item is not on view, change it
        QRectF mapped = m_view->currentItem()->mapRectToItem(parentItem(), QRectF(m_view->currentItem()->position(), m_view->currentItem()->size()));
        if (!QRectF(QPointF(0, 0), size()).intersects(mapped)) {
            m_view->setCurrentIndex(m_items.indexOf(m_firstVisibleItem));
        }
    });
}

ContentItem::~ContentItem()
{}

void ContentItem::setBoundedX(qreal x)
{
    if (!parentItem()) {
        return;
    }
    m_slideAnim->stop();
    setX(qBound(qMin(0.0, -width()+parentItem()->width()), x, 0.0));
}

void ContentItem::animateX(qreal newX)
{
    if (!parentItem()) {
        return;
    }

    const qreal to = qBound(qMin(0.0, -width()+parentItem()->width()), newX, 0.0);
    m_slideAnim->setStartValue(x());
    m_slideAnim->setEndValue(to);
    m_slideAnim->start();
}

qreal ContentItem::childWidth(QQuickItem *child)
{
    if (!parentItem()) {
        return 0.0;
    }

    if (m_columnResizeMode == ColumnsView::SingleColumn) {
        return parentItem()->width();

    } else if (m_columnResizeMode == ColumnsView::FixedColumns) {
        if (child == m_stretchableItem) {
            return qBound(m_columnWidth, (parentItem()->width() - m_columnWidth * m_reservedColumns), parentItem()->width());
        } else {
            return qMin(parentItem()->width(), m_columnWidth);
        }

    } else {
        //TODO:look for Layout size hints
        if (child->implicitWidth() > 0) {
            return qMin(parentItem()->width(), child->implicitWidth());
        }
        return qMin(parentItem()->width(), child->width());
    }
}

void ContentItem::layoutItems()
{
    qreal partialWidth = 0;
    for (QQuickItem *child : m_items) {
        child->setSize(QSizeF(childWidth(child), height()));
        child->setPosition(QPointF(partialWidth, 0.0));
        partialWidth += child->width();
    }
    setWidth(partialWidth);

    setBoundedX((m_firstVisibleItem ? -m_firstVisibleItem->x() : 0.0));
    setY(0);
}

void ContentItem::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    switch (change) {
    case QQuickItem::ItemChildAddedChange:
        connect(value.item, &QQuickItem::widthChanged, this, &ContentItem::layoutItems);
        if (!m_items.contains(value.item)) {
            m_items << value.item;
        }
        layoutItems();
        break;
    case QQuickItem::ItemChildRemovedChange: {
        disconnect(value.item, nullptr, this, nullptr);
        const int index = m_items.indexOf(value.item);
        m_items.removeAll(value.item);
        layoutItems();
        if (index < m_view->currentIndex()) {
            m_view->setCurrentIndex(qBound(0, index - 1, m_items.count() - 1));
        }
        break;
    }
    default:
        break;
    }
    QQuickItem::itemChange(change, value);
}







ColumnsView::ColumnsView(QQuickItem *parent)
    : QQuickItem(parent),
      m_contentItem(nullptr)
{
    //NOTE: this is to *not* trigger itemChange
    m_contentItem = new ContentItem(this);
    setAcceptedMouseButtons(Qt::LeftButton);
}

ColumnsView::~ColumnsView()
{
}

ColumnsView::ColumnResizeMode ColumnsView::columnResizeMode() const
{
    return m_contentItem->m_columnResizeMode;
}

void ColumnsView::setColumnResizeMode(ColumnResizeMode mode)
{
    if (m_contentItem->m_columnResizeMode == mode) {
        return;
    }

    m_contentItem->m_columnResizeMode = mode;
    m_contentItem->layoutItems();
    emit columnResizeModeChanged();
}

QQuickItem *ColumnsView::stretchableItem() const
{
    return m_contentItem->m_stretchableItem;
}

void ColumnsView::setStretchableItem(QQuickItem *item)
{
    if (m_contentItem->m_stretchableItem == item) {
        return;
    }

    m_contentItem->m_stretchableItem = item;
    m_contentItem->layoutItems();
    emit stretchableItemChanged();
}

qreal ColumnsView::columnWidth() const
{
    return m_contentItem->m_columnWidth;
}

void ColumnsView::setColumnWidth(qreal width)
{
    if (m_contentItem->m_columnWidth == width) {
        return;
    }

    m_contentItem->m_columnWidth = width;
    m_contentItem->layoutItems();
    emit columnWidthChanged();
}

int ColumnsView::reservedColumns() const
{
    return m_contentItem->m_reservedColumns;
}

void ColumnsView::setReservedColumns(int columns)
{
    if (m_contentItem->m_reservedColumns == columns) {
        return;
    }

    m_contentItem->m_reservedColumns = columns;
    m_contentItem->layoutItems();
    emit reservedColumnsChanged();
}

int ColumnsView::currentIndex() const
{
    return m_currentIndex;
}

void ColumnsView::setCurrentIndex(int index)
{
    if (!parentItem() || m_currentIndex == index || index < -1 || index >= m_contentItem->m_items.count()) {
        return;
    }

    m_currentIndex = index;

    if (index == -1) {
        m_currentItem.clear();
    } else {
        m_currentItem = m_contentItem->m_items[index];
        Q_ASSERT(m_currentItem);
        m_currentItem->forceActiveFocus();

        // If the current item is not on view, scroll
        QRectF mapped = m_currentItem->mapRectToItem(parentItem(), QRectF(m_currentItem->position(), m_currentItem->size()));
        if (!QRectF(QPointF(0, 0), parentItem()->size()).intersects(mapped)) {
            m_contentItem->m_firstVisibleItem = m_contentItem;
            m_contentItem->animateX(-m_contentItem->x());
        }
    }

    emit currentIndexChanged();
    emit currentItemChanged();
}

QQuickItem *ColumnsView::currentItem()
{
    return m_currentItem;
}



QQuickItem *ColumnsView::contentItem() const
{
    return m_contentItem;
}

void ColumnsView::addItem(QQuickItem *item)
{
    insertItem(m_contentItem->m_items.length(), item);
}

void ColumnsView::insertItem(int pos, QQuickItem *item)
{
    if (m_contentItem->m_items.contains(item)) {
        return;
    }
    m_contentItem->m_items.insert(qBound(0, pos, m_contentItem->m_items.length()), item);
    m_contentItem->m_firstVisibleItem = item;
    setCurrentIndex(pos);
    item->setParentItem(m_contentItem);
    item->forceActiveFocus();
    emit contentChildrenChanged();
}

void ColumnsView::moveItem(int from, int to)
{
    if (m_contentItem->m_items.isEmpty()
        || from < 0 || from >= m_contentItem->m_items.length()
        || to < 0 || to >= m_contentItem->m_items.length()) {
        return;
    }

    m_contentItem->m_items.move(from, to);
    m_contentItem->layoutItems();
}

void ColumnsView::removeItem(const QVariant &item)
{
    if (item.canConvert<QQuickItem *>()) {
        removeItem(item.value<QQuickItem *>());
    } else if (item.canConvert<int>()) {
        removeItem(item.toInt());
    }
}

void ColumnsView::removeItem(QQuickItem *item)
{
    if (m_contentItem->m_items.isEmpty() || !m_contentItem->m_items.contains(item)) {
        return;
    }

    m_contentItem->m_items.removeAll(item);

    if (QQmlEngine::objectOwnership(item) == QQmlEngine::JavaScriptOwnership) {
        item->deleteLater();
    } else {
        item->setParentItem(nullptr);
    }
}

void ColumnsView::removeItem(int pos)
{
    if (m_contentItem->m_items.isEmpty()
        || pos < 0 || pos >= m_contentItem->m_items.length()) {
        return;
    }

    removeItem(m_contentItem->m_items[pos]); 
}

void ColumnsView::clear()
{
    for (QQuickItem *item : m_contentItem->m_items) {
        if (QQmlEngine::objectOwnership(item) == QQmlEngine::JavaScriptOwnership) {
            item->deleteLater();
        } else {
            item->setParentItem(nullptr);
        }
    }
    m_contentItem->m_items.clear();
    emit contentChildrenChanged();
}


void ColumnsView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_contentItem->layoutItems();
    m_contentItem->setHeight(newGeometry.height());

    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void ColumnsView::mousePressEvent(QMouseEvent *event)
{
    m_oldMouseX = event->localPos().x();
    event->accept();
}

void ColumnsView::mouseMoveEvent(QMouseEvent *event)
{
    m_contentItem->setBoundedX(m_contentItem->x() + event->pos().x() - m_oldMouseX);
    m_oldMouseX = event->pos().x();
    event->accept();
}

void ColumnsView::mouseReleaseEvent(QMouseEvent *event)
{
    //TODO: animate
    QQuickItem *firstItem = m_contentItem->childAt(-m_contentItem->x(), 0);
    if (!firstItem) {
        return;
    }
    QQuickItem *nextItem = m_contentItem->childAt(firstItem->x() + firstItem->width() + 1, 0);

    //need to make the last item visible?
    if (nextItem && m_contentItem->width() - (-m_contentItem->x() + width()) < -m_contentItem->x() - firstItem->x()) {
        m_contentItem->m_firstVisibleItem = nextItem;
        m_contentItem->animateX(-nextItem->x());

    //The first one found?
    } else if (-m_contentItem->x() <= firstItem->x() + firstItem->width()/2 || !nextItem) {
        m_contentItem->m_firstVisibleItem = firstItem;
        m_contentItem->animateX(-firstItem->x());

    //the second?
    } else {
        m_contentItem->m_firstVisibleItem = nextItem;
        m_contentItem->animateX(-nextItem->x());
    }

    event->accept();
}

void ColumnsView::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    switch (change) {
    case QQuickItem::ItemChildAddedChange:
        if (m_contentItem && value.item != m_contentItem && !value.item->inherits("QQuickRepeater")) {
            addItem(value.item);
        }
        break;
    default:
        break;
    }
    QQuickItem::itemChange(change, value);
}

void ColumnsView::contentChildren_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *item)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return;
    }

    view->m_contentItem->m_items.append(item);
    item->setParentItem(view->m_contentItem);
}

int ColumnsView::contentChildren_count(QQmlListProperty<QQuickItem> *prop)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return 0;
    }

    return view->m_contentItem->m_items.count();
}

QQuickItem *ColumnsView::contentChildren_at(QQmlListProperty<QQuickItem> *prop, int index)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return nullptr;
    }

    if (index < 0 || index >= view->m_contentItem->m_items.count()) {
        return nullptr;
    }
    return view->m_contentItem->m_items.value(index);
}

void ColumnsView::contentChildren_clear(QQmlListProperty<QQuickItem> *prop)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return;
    }

    return view->m_contentItem->m_items.clear();
}

QQmlListProperty<QQuickItem> ColumnsView::contentChildren()
{
    return QQmlListProperty<QQuickItem>(this, nullptr,
                                     contentChildren_append,
                                     contentChildren_count,
                                     contentChildren_at,
                                     contentChildren_clear);
}

void ColumnsView::contentData_append(QQmlListProperty<QObject> *prop, QObject *object)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return;
    }

    view->m_contentData.append(object);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    //exclude repeaters from layout
    if (item && item->inherits("QQuickRepeater")) {
        item->setParentItem(view);
    } else if (item) {
        view->addItem(item);
    } else {
        object->setParent(view);
    }
}

int ColumnsView::contentData_count(QQmlListProperty<QObject> *prop)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return 0;
    }

    return view->m_contentData.count();
}

QObject *ColumnsView::contentData_at(QQmlListProperty<QObject> *prop, int index)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return nullptr;
    }

    if (index < 0 || index >= view->m_contentData.count()) {
        return nullptr;
    }
    return view->m_contentData.value(index);
}

void ColumnsView::contentData_clear(QQmlListProperty<QObject> *prop)
{
    ColumnsView *view = static_cast<ColumnsView *>(prop->object);
    if (!view) {
        return;
    }

    return view->m_contentData.clear();
}

QQmlListProperty<QObject> ColumnsView::contentData()
{
    return QQmlListProperty<QObject>(this, nullptr,
                                     contentData_append,
                                     contentData_count,
                                     contentData_at,
                                     contentData_clear);
}

#include "moc_columnsview.cpp"