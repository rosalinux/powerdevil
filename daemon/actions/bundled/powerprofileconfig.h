/*
 * Copyright 2020 Kai Uwe Broulik <kde@broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <powerdevilactionconfig.h>

class QComboBox;

namespace PowerDevil
{
namespace BundledActions
{
class PowerProfileConfig : public PowerDevil::ActionConfig
{
    Q_OBJECT
    Q_DISABLE_COPY(PowerProfileConfig)
public:
    PowerProfileConfig(QObject *parent, const QVariantList &args);
    ~PowerProfileConfig() override;

    void save() override;
    void load() override;
    QList<QPair<QString, QWidget *>> buildUi() override;

private:
    QComboBox *m_profileCombo = nullptr;
};

} // namespace BundledActions
} // namespace PowerDevil
