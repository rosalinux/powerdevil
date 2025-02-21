/********************************************************************
Copyright 2016  Martin Gräßlin <mgraesslin@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef POWERDEVIL_POWERMANAGEMENT_H
#define POWERDEVIL_POWERMANAGEMENT_H

#include <QObject>

namespace PowerDevil {

class Q_DECL_EXPORT PowerManagement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool canSuspend READ canSuspend NOTIFY canSuspendChanged)
    Q_PROPERTY(bool canHibernate READ canHibernate NOTIFY canHibernateChanged)
    Q_PROPERTY(bool canHybridSuspend READ canHybridSuspend NOTIFY canHybridSuspendChanged)
    Q_PROPERTY(bool canSuspendThenHibernate READ canSuspendThenHibernate NOTIFY canSuspendThenHibernateChanged)
public:
    ~PowerManagement() override;

    bool canSuspend() const;
    bool canHibernate() const;
    bool canHybridSuspend() const;
    bool canSuspendThenHibernate() const;

    static PowerManagement *instance();

public Q_SLOTS:
    void suspend();
    void hibernate();
    void hybridSuspend();
    void suspendThenHibernate();

Q_SIGNALS:
    void canSuspendChanged();
    void canSuspendThenHibernateChanged();
    void canHibernateChanged();
    void canHybridSuspendChanged();

protected:
    explicit PowerManagement();

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace PowerDevil

#endif // POWERDEVIL_POWERMANAGEMENT_H
