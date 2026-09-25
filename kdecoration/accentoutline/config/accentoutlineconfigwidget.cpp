/*
 * SPDX-FileCopyrightText: 2026 towgenik
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "accentoutlineconfigwidget.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QFormLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QSpinBox>
#include <QVBoxLayout>

Q_LOGGING_CATEGORY(lcAccentOutlineKcm, "org.towgenik.accentoutline.kcm", QtInfoMsg)

namespace AccentOutline
{
ConfigWidget::ConfigWidget(QObject *parent, const KPluginMetaData &data, const QVariantList &)
    : KCModule(parent, data)
{
    qCInfo(lcAccentOutlineKcm) << "Loading Accent Outline configuration module";
    auto *form = new QFormLayout;
    form->addRow(i18n("Outline width:"), m_outlineWidth = new QSpinBox(widget()));
    m_outlineWidth->setRange(0, 64);
    m_outlineWidth->setSuffix(i18n(" px"));
    m_outlineWidth->setToolTip(i18n("The outline is drawn outside the window and does not reduce the client area."));

    auto *help = new QLabel(i18n("The outline follows Plasma's resolved accent color. Enable “Accent color from wallpaper” in the Colors settings to use the "
                                 "wallpaper color. Set the width to 0 to hide the outline."),
                            widget());
    help->setWordWrap(true);

    auto *layout = new QVBoxLayout(widget());
    layout->addLayout(form);
    layout->addWidget(help);
    layout->addStretch();

    connect(m_outlineWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        updateChanged();
    });
}

void ConfigWidget::defaults()
{
    m_settings = std::make_unique<AccentOutline::AccentOutlineSettings>();
    m_settings->setDefaults();
    m_outlineWidth->setValue(m_settings->outlineWidth());
    setNeedsSave(true);
}

void ConfigWidget::load()
{
    m_settings = std::make_unique<AccentOutline::AccentOutlineSettings>();
    m_settings->load();
    qCInfo(lcAccentOutlineKcm) << "Loaded outline width:" << m_settings->outlineWidth();
    m_outlineWidth->setValue(m_settings->outlineWidth());
    setNeedsSave(false);
}

void ConfigWidget::save()
{
    if (!m_settings) {
        m_settings = std::make_unique<AccentOutline::AccentOutlineSettings>();
        m_settings->load();
    }

    m_settings->setOutlineWidth(m_outlineWidth->value());
    m_settings->save();
    qCInfo(lcAccentOutlineKcm) << "Saved outline width:" << m_settings->outlineWidth();
    setNeedsSave(false);

    const QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reloadConfig"));
    QDBusConnection::sessionBus().send(message);
}

void ConfigWidget::updateChanged()
{
    if (!m_settings) {
        return;
    }

    setNeedsSave(m_outlineWidth->value() != m_settings->outlineWidth());
}
}
