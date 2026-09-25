/*
 * SPDX-FileCopyrightText: 2026 towgenik
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "accentoutlineconfigwidget.h"

#include <KColorButton>
#include <KLocalizedString>

#include <QCheckBox>
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

    form->addRow(i18n("Rounded corners:"), m_roundedCorners = new QCheckBox(widget()));
    form->addRow(i18n("Corner radius:"), m_cornerRadius = new QSpinBox(widget()));
    m_cornerRadius->setRange(0, 32);
    m_cornerRadius->setSuffix(i18n(" px"));

    form->addRow(i18n("Use custom accent color:"), m_useCustomAccent = new QCheckBox(widget()));
    form->addRow(i18n("Custom accent color:"), m_customAccentColor = new KColorButton(widget()));

    auto *help = new QLabel(i18n("The outline requests Plasma's wallpaper accent color. If Plasma cannot provide one, the current palette accent is used. "
                                 "Enable a custom color to override both. Set the width to 0 to hide the outline."),
                            widget());
    help->setWordWrap(true);

    auto *layout = new QVBoxLayout(widget());
    layout->addLayout(form);
    layout->addWidget(help);
    layout->addStretch();

    connect(m_outlineWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        updateChanged();
    });
    connect(m_cornerRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        updateChanged();
    });
    connect(m_roundedCorners, &QCheckBox::toggled, this, [this](bool checked) {
        m_cornerRadius->setEnabled(checked);
        updateChanged();
    });
    connect(m_useCustomAccent, &QCheckBox::toggled, this, [this](bool checked) {
        m_customAccentColor->setEnabled(checked);
        updateChanged();
    });
    connect(m_customAccentColor, &KColorButton::changed, this, [this](const QColor &) {
        updateChanged();
    });
}

void ConfigWidget::defaults()
{
    m_settings = std::make_unique<AccentOutline::AccentOutlineSettings>();
    m_settings->setDefaults();
    m_outlineWidth->setValue(m_settings->outlineWidth());
    m_roundedCorners->setChecked(m_settings->roundedCorners());
    m_cornerRadius->setValue(m_settings->cornerRadius());
    m_useCustomAccent->setChecked(m_settings->useCustomAccent());
    m_customAccentColor->setColor(m_settings->customAccentColor());
    m_cornerRadius->setEnabled(m_roundedCorners->isChecked());
    m_customAccentColor->setEnabled(m_useCustomAccent->isChecked());
    setNeedsSave(true);
}

void ConfigWidget::load()
{
    m_settings = std::make_unique<AccentOutline::AccentOutlineSettings>();
    m_settings->load();
    qCInfo(lcAccentOutlineKcm) << "Loaded outline settings: width" << m_settings->outlineWidth() << "rounded" << m_settings->roundedCorners() << "radius"
                               << m_settings->cornerRadius() << "customAccent" << m_settings->useCustomAccent();
    m_outlineWidth->setValue(m_settings->outlineWidth());
    m_roundedCorners->setChecked(m_settings->roundedCorners());
    m_cornerRadius->setValue(m_settings->cornerRadius());
    m_useCustomAccent->setChecked(m_settings->useCustomAccent());
    m_customAccentColor->setColor(m_settings->customAccentColor());
    m_cornerRadius->setEnabled(m_roundedCorners->isChecked());
    m_customAccentColor->setEnabled(m_useCustomAccent->isChecked());
    setNeedsSave(false);
}

void ConfigWidget::save()
{
    if (!m_settings) {
        m_settings = std::make_unique<AccentOutline::AccentOutlineSettings>();
        m_settings->load();
    }

    m_settings->setOutlineWidth(m_outlineWidth->value());
    m_settings->setRoundedCorners(m_roundedCorners->isChecked());
    m_settings->setCornerRadius(m_cornerRadius->value());
    m_settings->setUseCustomAccent(m_useCustomAccent->isChecked());
    m_settings->setCustomAccentColor(m_customAccentColor->color());
    m_settings->save();
    qCInfo(lcAccentOutlineKcm) << "Saved outline settings: width" << m_settings->outlineWidth() << "rounded" << m_settings->roundedCorners() << "radius"
                               << m_settings->cornerRadius() << "customAccent" << m_settings->useCustomAccent();
    setNeedsSave(false);

    const QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reloadConfig"));
    QDBusConnection::sessionBus().send(message);

    const QDBusMessage outlineReload =
        QDBusMessage::createSignal(QStringLiteral("/AccentOutline"), QStringLiteral("io.github.towgenik.AccentOutline"), QStringLiteral("reloadConfig"));
    QDBusConnection::sessionBus().send(outlineReload);
}

void ConfigWidget::updateChanged()
{
    if (!m_settings) {
        return;
    }

    const bool modified = m_outlineWidth->value() != m_settings->outlineWidth() || m_roundedCorners->isChecked() != m_settings->roundedCorners()
        || m_cornerRadius->value() != m_settings->cornerRadius() || m_useCustomAccent->isChecked() != m_settings->useCustomAccent()
        || m_customAccentColor->color() != m_settings->customAccentColor();
    setNeedsSave(modified);
}
}
