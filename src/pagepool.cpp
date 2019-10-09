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

#include "pagepool.h"

#include <QDebug>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>

PagePool::PagePool(QObject *parent)
    : QObject(parent)
{
}

PagePool::~PagePool()
{
}

QUrl PagePool::lastLoadedUrl() const
{
    return m_lastLoadedUrl;
}

void PagePool::setCachePages(bool cache)
{
    if (cache == m_cachePages) {
        return;
    }

    if (cache) {
        for (auto *c : m_componentForUrl.values()) {
            c->deleteLater();
        }
        m_componentForUrl.clear();

        for (auto *i : m_itemForUrl.values()) {
            // items that had been deparented are safe to delete
            if (!i->parentItem()) {
                i->deleteLater();
            }
            QQmlEngine::setObjectOwnership(i, QQmlEngine::JavaScriptOwnership);
        }
        m_itemForUrl.clear();
    }

    m_cachePages = cache;
    emit cachePagesChanged();
}

bool PagePool::cachePages() const
{
    return m_cachePages;
}

QQuickItem *PagePool::loadPage(const QString &url, QJSValue callback)
{
    Q_ASSERT(qmlEngine(this));
    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    Q_ASSERT(ctx);

    const QUrl actualUrl = resolvedUrl(url);

    QQuickItem *foundItem = nullptr;
    if (actualUrl == m_lastLoadedUrl && m_lastLoadedItem) {
        foundItem = m_lastLoadedItem;
    } else if (m_itemForUrl.contains(actualUrl)) {
        foundItem = m_itemForUrl[actualUrl];
    }

    if (foundItem) {
        if (callback.isCallable()) {
            QJSValueList args = {qmlEngine(this)->newQObject(foundItem)};
            callback.call(args);
            m_lastLoadedUrl = actualUrl;
            emit lastLoadedUrlChanged();
            // We could return the item, but for api coherence return null
            return nullptr;
        } else {
            m_lastLoadedUrl = actualUrl;
            emit lastLoadedUrlChanged();
            return foundItem;
        }
    }

    QQmlComponent *component = m_componentForUrl.value(actualUrl);

    if (!component) {
        component = new QQmlComponent(qmlEngine(this), actualUrl, QQmlComponent::PreferSynchronous);
    }

    if (component->status() == QQmlComponent::Loading) {
        if (!callback.isCallable()) {
            component->deleteLater();
            m_componentForUrl.remove(actualUrl);
            return nullptr;
        }

        connect(component, &QQmlComponent::statusChanged, this,
                [this, component, callback] (QQmlComponent::Status status) mutable {
            if (status != QQmlComponent::Ready) {
                qWarning() << component->errors();
                m_componentForUrl.remove(component->url());
                component->deleteLater();
                return;
            }
            QQuickItem *item = createFromComponent(component);
            if (item) {
                QJSValueList args = {qmlEngine(this)->newQObject(item)};
                callback.call(args);
            }

            if (m_cachePages) {
                component->deleteLater();
            } else {
                m_componentForUrl[component->url()] = component;
            }
        });

        return nullptr;

    } else if (component->status() != QQmlComponent::Ready) {
        qWarning() << component->errors();
        return nullptr;
    }

    QQuickItem *item = createFromComponent(component);
    if (m_cachePages) {
        component->deleteLater();
    } else {
        m_componentForUrl[component->url()] = component;
    }

    if (callback.isCallable()) {
        QJSValueList args = {qmlEngine(this)->newQObject(item)};
        callback.call(args);
        m_lastLoadedUrl = actualUrl;
        emit lastLoadedUrlChanged();
        // We could return the item, but for api coherence return null
        return nullptr;
    } else {
        m_lastLoadedUrl = actualUrl;
        emit lastLoadedUrlChanged();
        return item;
    }
}

QQuickItem *PagePool::createFromComponent(QQmlComponent *component)
{
    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    Q_ASSERT(ctx);

    QObject *obj = component->create(ctx);
    // Error?
    if (!obj) {
        return nullptr;
    }

    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (!item) {
        obj->deleteLater();
        return nullptr;
    }

    // Always cache just the last one
    m_lastLoadedItem = item;

    if (m_cachePages) {
        QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
        m_itemForUrl[component->url()] = item;
    } else {
        QQmlEngine::setObjectOwnership(item, QQmlEngine::JavaScriptOwnership);
    }

    return item;
}

QUrl PagePool::resolvedUrl(const QString &stringUrl) const
{
    Q_ASSERT(qmlEngine(this));
    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    Q_ASSERT(ctx);

    QUrl actualUrl(stringUrl);
    if (actualUrl.scheme().isEmpty()) {
        actualUrl = ctx->resolvedUrl(actualUrl);
    }
    return actualUrl;
}

bool PagePool::isLocalUrl(const QUrl &url)
{
    return url.isLocalFile() || url.scheme().isEmpty() || url.scheme() == QStringLiteral("qrc");
}

QUrl PagePool::urlForPage(QQuickItem *item) const
{
    return m_urlForItem.value(item);
}

bool PagePool::contains(const QVariant &page) const
{
    if (page.canConvert<QQuickItem *>()) {
        return m_urlForItem.contains(page.value<QQuickItem *>());
    } else if (page.canConvert<QString>()) {
        const QUrl actualUrl = resolvedUrl(page.value<QString>());
        return m_itemForUrl.contains(actualUrl);
    } else {
        return false;
    }
}

void PagePool::deletePage(const QVariant &page)
{
    if (!contains(page)) {
        return;
    }

    QQuickItem *item;
    if (page.canConvert<QQuickItem *>()) {
        item = page.value<QQuickItem *>();
    } else if (page.canConvert<QString>()) {
        QString url = page.value<QString>();
        if (url.isEmpty()) {
            return;
        }
        const QUrl actualUrl = resolvedUrl(page.value<QString>());

        item = m_itemForUrl.value(actualUrl);
    } else {
        return;
    }

    if (!item) {
        return;
    }

    const QUrl url = m_urlForItem.value(item);

    if (url.isEmpty()) {
        return;
    }

    m_itemForUrl.remove(url);
    m_urlForItem.remove(item);
    item->deleteLater();
}


#include "moc_pagepool.cpp"