/*
 * SPDX-FileCopyrightText: 2026 towgenik
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "accentoutlinesettings.h"

#include <KCModule>
#include <KPluginMetaData>

#include <QVariantList>
#include <QWidget>

#include <memory>

class QSpinBox;

namespace AccentOutline
{
class ConfigWidget : public KCModule
{
    Q_OBJECT

public:
    explicit ConfigWidget(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    void defaults() override;
    void load() override;
    void save() override;

private:
    void updateChanged();

    QSpinBox *m_outlineWidth = nullptr;
    std::unique_ptr<AccentOutline::AccentOutlineSettings> m_settings;
};
}
