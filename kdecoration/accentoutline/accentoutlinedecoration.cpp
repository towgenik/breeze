/*
 * SPDX-FileCopyrightText: 2026 towgenik
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "accentoutlinedecoration.h"

#include <KConfigGroup>
#include <KPluginFactory>

#include <KDecoration3/ScaleHelpers>

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QMarginsF>
#include <QPainter>

#include <algorithm>

K_PLUGIN_FACTORY_WITH_JSON(AccentOutlineFactory, "accentoutline.json", registerPlugin<AccentOutline::Decoration>();)

namespace AccentOutline
{
AccentColorProvider *AccentColorProvider::instance()
{
    // Keep one D-Bus request per KWin process instead of repeating the
    // potentially expensive wallpaper extraction for every decorated window.
    static AccentColorProvider *provider = new AccentColorProvider;
    return provider;
}

AccentColorProvider::AccentColorProvider()
{
    auto dbus = QDBusConnection::sessionBus();
    dbus.connect(QStringLiteral("org.kde.plasmashell"),
                 QStringLiteral("/PlasmaShell"),
                 QStringLiteral("org.kde.PlasmaShell"),
                 QStringLiteral("wallpaperChanged"),
                 this,
                 SLOT(onWallpaperChanged(quint32)));
    dbus.connect(QStringLiteral("org.kde.plasmashell"),
                 QStringLiteral("/PlasmaShell"),
                 QStringLiteral("org.kde.PlasmaShell"),
                 QStringLiteral("colorChanged"),
                 this,
                 SLOT(onColorChanged(QString)));

    requestColor();
}

void AccentColorProvider::onWallpaperChanged(quint32)
{
    requestColor();
}

void AccentColorProvider::onColorChanged(const QString &color)
{
    const QColor accent = QColor::fromString(color);
    if (accent.isValid() && accent.alpha() > 0) {
        m_color = accent;
        Q_EMIT colorChanged();
    }
}

void AccentColorProvider::requestColor()
{
    if (m_watcher) {
        return;
    }

    const QDBusMessage request = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                                QStringLiteral("/PlasmaShell"),
                                                                QStringLiteral("org.kde.PlasmaShell"),
                                                                QStringLiteral("color"));
    m_watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(request), this);
    connect(m_watcher, &QDBusPendingCallWatcher::finished, this, [this] {
        auto *watcher = m_watcher;
        m_watcher = nullptr;

        const QDBusPendingReply<quint32> reply = *watcher;
        if (reply.isValid()) {
            const QColor accent = QColor::fromRgba(reply.value());
            if (accent.isValid() && accent.alpha() > 0) {
                m_color = accent;
                Q_EMIT colorChanged();
            }
        }

        watcher->deleteLater();
    });
}

Decoration::Decoration(QObject *parent, const QVariantList &args)
    : KDecoration3::Decoration(parent, args)
{
}

void Decoration::paint(QPainter *, const QRectF &)
{
    // The outline is rendered by KWin through Decoration::setBorderOutline().
    // Keeping paint() empty avoids drawing the outline twice.
}

bool Decoration::init()
{
    m_kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    m_kdeGlobalsWatcher = KConfigWatcher::create(m_kdeGlobals);

    auto dbus = QDBusConnection::sessionBus();
    dbus.connect(QString(),
                 QStringLiteral("/KGlobalSettings"),
                 QStringLiteral("org.kde.KGlobalSettings"),
                 QStringLiteral("notifyChange"),
                 this,
                 SLOT(updateDecoration()));

    m_accentColorProvider = AccentColorProvider::instance();
    connect(m_accentColorProvider, &AccentColorProvider::colorChanged, this, &Decoration::updateDecoration);

    connect(m_kdeGlobalsWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.name() != QLatin1String("General")) {
            return;
        }

        if (names.contains(QByteArrayLiteral("AccentColor")) || names.contains(QByteArrayLiteral("accentColorFromWallpaper"))
            || names.contains(QByteArrayLiteral("ColorScheme"))) {
            updateDecoration();
        }
    });

    reconfigure();

    if (settings()) {
        connect(settings().get(), &KDecoration3::DecorationSettings::reconfigured, this, &Decoration::reconfigure);
    }

    connect(window(), &KDecoration3::DecoratedWindow::activeChanged, this, &Decoration::updateDecoration);
    connect(window(), &KDecoration3::DecoratedWindow::paletteChanged, this, &Decoration::updateDecoration);
    connect(window(), &KDecoration3::DecoratedWindow::maximizedChanged, this, &Decoration::updateDecoration);
    connect(window(), &KDecoration3::DecoratedWindow::maximizedHorizontallyChanged, this, &Decoration::updateDecoration);
    connect(window(), &KDecoration3::DecoratedWindow::maximizedVerticallyChanged, this, &Decoration::updateDecoration);
    connect(window(), &KDecoration3::DecoratedWindow::adjacentScreenEdgesChanged, this, &Decoration::updateDecoration);
    connect(window(), &KDecoration3::DecoratedWindow::nextScaleChanged, this, &Decoration::updateDecoration);

    setShadow(nullptr);
    updateDecoration();
    return true;
}

void Decoration::reconfigure()
{
    m_settings = std::make_unique<AccentOutline::AccentOutlineSettings>();
    m_settings->load();

    if (m_settings) {
        connect(m_settings.get(), &KConfigSkeleton::configChanged, this, &Decoration::updateDecoration);
    }

    updateDecoration();
}

void Decoration::updateDecoration()
{
    if (!window()) {
        return;
    }

    const int configuredWidth = m_settings ? m_settings->outlineWidth() : 8;
    const qreal scale = window()->nextScale() > 0 ? window()->nextScale() : 1;
    const qreal outlineWidth = KDecoration3::snapToPixelGrid(configuredWidth, scale);
    const bool maximized = window()->isMaximized();
    const qreal resizeWidth = maximized ? 0 : qMax<qreal>(outlineWidth, settings() ? settings()->largeSpacing() : 0);

    // There is deliberately no titlebar or button area. The resize-only
    // margins keep the edges usable without consuming client-area space,
    // including when the visible outline is disabled.
    setTitleBar(QRectF());
    setBorders(QMarginsF());
    setResizeOnlyBorders(maximized ? QMarginsF() : QMarginsF(resizeWidth, resizeWidth, resizeWidth, resizeWidth));

    if (configuredWidth <= 0 || maximized) {
        setBorderRadius(KDecoration3::BorderRadius());
        setBorderOutline(KDecoration3::BorderOutline());
    } else {
        // Keep the corners visually tied to the outline without introducing a
        // second, separately painted frame.
        const qreal radius = std::clamp<qreal>(outlineWidth, 4.0, 16.0);
        setBorderRadius(KDecoration3::BorderRadius(radius));
        setBorderOutline(KDecoration3::BorderOutline(outlineWidth, resolvedAccentColor(), KDecoration3::BorderRadius(radius)));
    }

    setShadow(nullptr);
    update();
}

QColor Decoration::resolvedAccentColor() const
{
    // PlasmaShell provides the actual dominant color even when the global
    // wallpaper-accent setting is disabled. The call is asynchronous and the
    // cached value is retained if plasmashell is unavailable.
    if (m_accentColorProvider) {
        const QColor wallpaperAccent = m_accentColorProvider->color();
        if (wallpaperAccent.isValid() && wallpaperAccent.alpha() > 0) {
            return wallpaperAccent;
        }
    }

    if (m_kdeGlobals) {
        const KConfigGroup colors(m_kdeGlobals, QStringLiteral("General"));
        const bool fromWallpaper = colors.readEntry(QStringLiteral("accentColorFromWallpaper"), false);
        const QColor configuredAccent = colors.readEntry(QStringLiteral("AccentColor"), QColor());

        // Plasma writes the extracted wallpaper color here when wallpaper
        // accent mode is enabled. Respect that mode instead of treating a
        // user-selected custom accent as a wallpaper color.
        if (fromWallpaper && configuredAccent.isValid() && configuredAccent.alpha() > 0) {
            return configuredAccent;
        }
    }

    const QPalette::ColorGroup group = window()->isActive() ? QPalette::Active : QPalette::Inactive;
    const QColor paletteAccent = window()->palette().color(group, QPalette::Accent);
    if (paletteAccent.isValid()) {
        return paletteAccent;
    }

    const QColor paletteHighlight = window()->palette().color(group, QPalette::Highlight);
    if (paletteHighlight.isValid()) {
        return paletteHighlight;
    }

    return QColor(61, 174, 233);
}
}

#include "accentoutlinedecoration.moc"
