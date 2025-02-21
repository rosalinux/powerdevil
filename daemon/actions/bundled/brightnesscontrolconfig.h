/***************************************************************************
 *   Copyright (C) 2010 by Dario Freddi <drf@kde.org>                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#ifndef POWERDEVIL_BUNDLEDACTIONS_BRIGHTNESSCONTROLCONFIG_H
#define POWERDEVIL_BUNDLEDACTIONS_BRIGHTNESSCONTROLCONFIG_H

#include <powerdevilactionconfig.h>

class QSlider;
class QLabel;

namespace PowerDevil {
namespace BundledActions {

class Q_DECL_EXPORT BrightnessControlConfig : public PowerDevil::ActionConfig
{
    Q_OBJECT
    Q_DISABLE_COPY(BrightnessControlConfig)
public:
    BrightnessControlConfig(QObject*, const QVariantList&);
    ~BrightnessControlConfig() override;

    void save() override;
    void load() override;
    QList< QPair< QString, QWidget* > > buildUi() override;

private:
    QSlider *m_slider;
    QLabel *m_text;
};

}
}

#endif // POWERDEVIL_BUNDLEDACTIONS_BRIGHTNESSCONTROLCONFIG_H
