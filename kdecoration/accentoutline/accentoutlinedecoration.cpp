/*
 * SPDX-FileCopyrightText: 2026 towgenik
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "accentoutlinedecoration.h"

#include <KConfigGroup>
#include <KPluginFactory>

#include <KDecoration3/ScaleHelpers>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QLoggingCategory>
#include <QMarginsF>
#include <QPainter>

#include <algorithm>

Q_LOGGING_CATEGORY(lcAccentOutline, "org.towgenik.accentoutline", QtInfoMsg)

namespace
{
bool debugEnabled()
{
    static const bool enabled = qEnvironmentVariableIsSet("ACCENT_OUTLINE_DEBUG");
    return enabled;
}
}

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
    qCInfo(lcAccentOutline) << "Accent color provider initialized";
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
    qCDebug(lcAccentOutline) << "Plasma reported a wallpaper change";
    requestColor();
}

void AccentColorProvider::onColorChanged(const QString &color)
{
    const QColor accent = QColor::fromString(color);
    if (accent.isValid() && accent.alpha() > 0) {
        qCDebug(lcAccentOutline) << "Plasma accent color changed to" << accent.name();
        m_color = accent;
        Q_EMIT colorChanged();
    }
}

void AccentColorProvider::requestColor()
{
    if (m_watcher) {
        qCDebug(lcAccentOutline) << "Accent color request already pending";
        return;
    }

    qCDebug(lcAccentOutline) << "Requesting wallpaper accent from PlasmaShell";
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
                qCInfo(lcAccentOutline) << "Resolved wallpaper accent:" << accent.name();
                m_color = accent;
                Q_EMIT colorChanged();
            } else {
                qCWarning(lcAccentOutline) << "PlasmaShell returned a transparent or invalid accent";
            }
        } else {
            qCWarning(lcAccentOutline) << "PlasmaShell accent request failed:" << reply.error().message();
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
    static bool pathsLogged = false;
    if (!pathsLogged) {
        qCInfo(lcAccentOutline) << "QT_PLUGIN_PATH=" << qEnvironmentVariable("QT_PLUGIN_PATH", "<unset>");
        qCInfo(lcAccentOutline) << "Qt library paths:" << QCoreApplication::libraryPaths();
        pathsLogged = true;
    }

    m_kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    m_kdeGlobalsWatcher = KConfigWatcher::create(m_kdeGlobals);

    auto dbus = QDBusConnection::sessionBus();
    dbus.connect(QString(),
                 QStringLiteral("/KGlobalSettings"),
                 QStringLiteral("org.kde.KGlobalSettings"),
                 QStringLiteral("notifyChange"),
                 this,
                 SLOT(updateDecoration()));
    if (!dbus.connect(QString(),
                      QStringLiteral("/AccentOutline"),
                      QStringLiteral("io.github.towgenik.AccentOutline"),
                      QStringLiteral("reloadConfig"),
                      this,
                      SLOT(reconfigure()))) {
        qCWarning(lcAccentOutline) << "Could not connect to the Accent Outline reload signal";
    }

    m_outlineConfig = KSharedConfig::openConfig(QStringLiteral("accentoutlinerc"));
    m_outlineConfigWatcher = KConfigWatcher::create(m_outlineConfig);
    connect(m_outlineConfigWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        const bool relevant = names.isEmpty() || names.contains(QByteArrayLiteral("OutlineWidth")) || names.contains(QByteArrayLiteral("RoundedCorners"))
            || names.contains(QByteArrayLiteral("CornerRadius")) || names.contains(QByteArrayLiteral("UseCustomAccent"))
            || names.contains(QByteArrayLiteral("CustomAccentColor"));
        if (group.name() == QLatin1String("Common") && relevant) {
            qCDebug(lcAccentOutline) << "Outline configuration changed; reloading";
            reconfigure();
        }
    });

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
    qCInfo(lcAccentOutline) << "Initialized decoration for" << window()->windowClass() << "scale" << window()->nextScale();
    return true;
}

void Decoration::reconfigure()
{
    m_outlineSettings = std::make_unique<AccentOutline::AccentOutlineSettings>();
    m_outlineSettings->load();
    qCInfo(lcAccentOutline) << "Loaded outline settings: width" << m_outlineSettings->outlineWidth() << "rounded" << m_outlineSettings->roundedCorners()
                            << "radius" << m_outlineSettings->cornerRadius() << "customAccent" << m_outlineSettings->useCustomAccent();

    if (m_outlineSettings) {
        connect(m_outlineSettings.get(), &KConfigSkeleton::configChanged, this, &Decoration::updateDecoration);
    }

    updateDecoration();
}

void Decoration::updateDecoration()
{
    if (!window()) {
        return;
    }

    const int configuredWidth = m_outlineSettings ? m_outlineSettings->outlineWidth() : 8;
    const qreal scale = window()->nextScale() > 0 ? window()->nextScale() : 1;
    const qreal outlineWidth = KDecoration3::snapToPixelGrid(configuredWidth, scale);
    const bool maximized = window()->isMaximized();
    const bool focused = window()->isActive();
    const qreal resizeWidth = maximized ? 0 : qMax<qreal>(KDecoration3::pixelSize(scale), settings() ? settings()->largeSpacing() : 0);

    // There is deliberately no titlebar or button area. The resize-only
    // margins keep the edges usable without consuming client-area space,
    // including when the visible outline is disabled.
    setTitleBar(QRectF());
    setBorders(QMarginsF());
    setResizeOnlyBorders(maximized ? QMarginsF() : QMarginsF(resizeWidth, resizeWidth, resizeWidth, resizeWidth));

    // KWin turns the border radius into a shader mask on the window contents
    // (Window::updateDecorationBorderRadius() -> Item::borderRadius() ->
    // cornerStack in the scene renderer), which is completely independent of
    // the BorderOutline. Only the outline is focus-dependent, so unfocused
    // windows keep their rounded corners. This mirrors how Breeze separates
    // the two in breezedecoration.cpp.
    const bool rounded = m_outlineSettings ? m_outlineSettings->roundedCorners() : true;
    const int configuredRadius = m_outlineSettings ? m_outlineSettings->cornerRadius() : 8;
    const qreal radius = rounded ? std::max<qreal>(KDecoration3::snapToPixelGrid(configuredRadius, scale), 0) : 0;

    setBorderRadius(maximized ? KDecoration3::BorderRadius() : KDecoration3::BorderRadius(radius));

    if (configuredWidth <= 0 || maximized || !focused) {
        setBorderOutline(KDecoration3::BorderOutline());
    } else {
        const bool useCustomAccent = m_outlineSettings && m_outlineSettings->useCustomAccent();
        const QColor customAccent = m_outlineSettings ? m_outlineSettings->customAccentColor() : QColor();
        const QColor outlineColor = useCustomAccent && customAccent.isValid() && customAccent.alpha() > 0 ? customAccent : resolvedAccentColor();

        if (useCustomAccent && customAccent.isValid() && customAccent.alpha() > 0) {
            qCDebug(lcAccentOutline) << "Accent source: custom configuration" << customAccent.name();
        }

        setBorderOutline(KDecoration3::BorderOutline(outlineWidth, outlineColor, KDecoration3::BorderRadius(radius)));
    }

    setShadow(nullptr);
    if (debugEnabled()) {
        const auto outline = borderOutline();
        qCInfo(lcAccentOutline) << "geometry: borders" << borders() << "resizeOnly" << resizeOnlyBorders() << "titleBar" << titleBar() << "maximized"
                                << maximized << "focused" << focused << "outlineNull" << outline.isNull() << "thickness" << outline.thickness() << "radius"
                                << borderRadius().topLeft() << "color" << outline.color().name() << "shadow" << (shadow() ? "present" : "null");
    }
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
            qCDebug(lcAccentOutline) << "Accent source: PlasmaShell wallpaper" << wallpaperAccent.name();
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
            qCDebug(lcAccentOutline) << "Accent source: kdeglobals AccentColor" << configuredAccent.name();
            return configuredAccent;
        }
    }

    const QPalette::ColorGroup group = window()->isActive() ? QPalette::Active : QPalette::Inactive;
    const QColor paletteAccent = window()->palette().color(group, QPalette::Accent);
    if (paletteAccent.isValid()) {
        qCDebug(lcAccentOutline) << "Accent source: window QPalette::Accent" << paletteAccent.name();
        return paletteAccent;
    }

    const QColor paletteHighlight = window()->palette().color(group, QPalette::Highlight);
    if (paletteHighlight.isValid()) {
        qCDebug(lcAccentOutline) << "Accent source: window QPalette::Highlight" << paletteHighlight.name();
        return paletteHighlight;
    }

    qCWarning(lcAccentOutline) << "No accent color available; using fallback blue";
    return QColor(61, 174, 233);
}
}

#include "accentoutlinedecoration.moc"
