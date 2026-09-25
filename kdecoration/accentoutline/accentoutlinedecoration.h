/*
 * SPDX-FileCopyrightText: 2026 towgenik
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "accentoutlinesettings.h"

#include <KConfigWatcher>
#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationSettings>
#include <KSharedConfig>

#include <QColor>
#include <QObject>
#include <QVariantList>

#include <memory>

class QDBusPendingCallWatcher;

namespace AccentOutline
{
class AccentColorProvider : public QObject
{
    Q_OBJECT

public:
    static AccentColorProvider *instance();

    QColor color() const
    {
        return m_color;
    }

Q_SIGNALS:
    void colorChanged();

private Q_SLOTS:
    void onWallpaperChanged(quint32 screenNumber);
    void onColorChanged(const QString &color);

private:
    AccentColorProvider();

    void requestColor();

    QDBusPendingCallWatcher *m_watcher = nullptr;
    QColor m_color;
};

class Decoration : public KDecoration3::Decoration
{
    Q_OBJECT

public:
    explicit Decoration(QObject *parent = nullptr, const QVariantList &args = {});

    void paint(QPainter *painter, const QRectF &repaintRegion) override;

    bool init() override;

private Q_SLOTS:
    void updateDecoration();

private:
    void reconfigure();
    QColor resolvedAccentColor() const;

    std::unique_ptr<AccentOutline::AccentOutlineSettings> m_settings;
    KSharedConfig::Ptr m_kdeGlobals;
    KConfigWatcher::Ptr m_kdeGlobalsWatcher;
    AccentColorProvider *m_accentColorProvider = nullptr;
};
}
