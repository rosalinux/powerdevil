/*
 * Copyright 2020 Kai Uwe Broulik <kde@broulik.de>
 * Copyright 2021 David Redondo <kde@david-redondo.de>
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

#include "powerprofile.h"

#include "power_profiles_interface.h"
#include "powerprofileadaptor.h"
#include "properties_interface.h"

#include <powerdevil_debug.h>

#include <KConfigGroup>
#include <KPluginFactory>

using namespace PowerDevil::BundledActions;

K_PLUGIN_CLASS_WITH_JSON(PowerProfile, "powerdevilpowerprofileaction.json")

static const QString activeProfileProperty = QStringLiteral("ActiveProfile");
static const QString profilesProperty = QStringLiteral("Profiles");
static const QString performanceInhibitedProperty = QStringLiteral("PerformanceInhibited");
static const QString performanceDegradedProperty = QStringLiteral("PerformanceDegraded");
static const QString profileHoldsProperty = QStringLiteral("ActiveProfileHolds");

static const QString ppdName = QStringLiteral("net.hadess.PowerProfiles");
static const QString ppdPath = QStringLiteral("/net/hadess/PowerProfiles");

PowerProfile::PowerProfile(QObject *parent, const QVariantList &)
    : Action(parent)
    , m_powerProfilesInterface(new NetHadessPowerProfilesInterface(ppdName, ppdPath, QDBusConnection::systemBus(), this))
    , m_powerProfilesPropertiesInterface(new OrgFreedesktopDBusPropertiesInterface(ppdName, ppdPath, QDBusConnection::systemBus(), this))
    , m_holdWatcher(new QDBusServiceWatcher(QString(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration, this))
{
    new PowerProfileAdaptor(this);

    connect(m_holdWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &PowerProfile::serviceUnregistered);
    connect(m_powerProfilesPropertiesInterface, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged, this, &PowerProfile::propertiesChanged);
    connect(m_powerProfilesInterface, &NetHadessPowerProfilesInterface::ProfileReleased, this, [this] (unsigned int cookie) {
        auto it = std::find(m_holdMap.begin(), m_holdMap.end(), cookie);
        if (it != m_holdMap.end()) {
            if (m_holdMap.count(it.key()) == 1) {
                m_holdWatcher->removeWatchedService(it.key());
            }
            m_holdMap.erase(it);
        }
    });

    auto watcher = new QDBusPendingCallWatcher(m_powerProfilesPropertiesInterface->GetAll(m_powerProfilesInterface->interface()));
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        watcher->deleteLater();
        QDBusPendingReply<QVariantMap> reply = *watcher;
        if (watcher->isError()) {
            return;
        }
        readProperties(reply.value());
    });
    qDBusRegisterMetaType<QList<QVariantMap>>();
}

PowerProfile::~PowerProfile() = default;

void PowerProfile::onProfileLoad()
{
    if (!m_configuredProfile.isEmpty()) {
        setProfile(m_configuredProfile);
    }
}

void PowerProfile::onWakeupFromIdle()
{
}

void PowerProfile::onIdleTimeout(int msec)
{
    Q_UNUSED(msec);
}

void PowerProfile::onProfileUnload()
{
}

void PowerProfile::triggerImpl(const QVariantMap &args)
{
    Q_UNUSED(args);
}

bool PowerProfile::loadAction(const KConfigGroup &config)
{
    if (config.hasKey("profile")) {
        m_configuredProfile = config.readEntry("profile", QString());
    }

    return true;
}

bool PowerProfile::isSupported()
{
    return QDBusConnection::systemBus().interface()->activatableServiceNames().value().contains(ppdName);
}

QStringList PowerProfile::profileChoices() const
{
    return m_profileChoices;
}

QString PowerProfile::currentProfile() const
{
    return m_currentProfile;
}

QString PowerProfile::performanceDegradedReason() const
{
    return m_degradationReason;
}

QString PowerProfile::performanceInhibitedReason() const
{
    return m_performanceInhibitedReason;
}

QList<QVariantMap> PowerProfile::profileHolds() const
{
    return m_profileHolds;
}

void PowerProfile::setProfile(const QString &profile)
{
    auto call = m_powerProfilesPropertiesInterface->Set(m_powerProfilesInterface->interface(), activeProfileProperty, QDBusVariant(profile));
    if (calledFromDBus()) {
        setDelayedReply(true);
        const auto msg = message();
        auto watcher = new QDBusPendingCallWatcher(call);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [msg, watcher] {
            watcher->deleteLater();
            if (watcher->isError()) {
                QDBusConnection::sessionBus().send(msg.createErrorReply(watcher->error()));
            } else {
                QDBusConnection::sessionBus().send(msg.createReply());
            }
        });
    }
}

unsigned int PowerProfile::holdProfile(const QString& profile, const QString& reason, const QString& applicationId)
{
    if (!m_profileChoices.contains(profile)) {
        sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("%1 is not a valid profile").arg(profile));
        return 0; // ignored by QtDBus
    }
    setDelayedReply(true);
    const auto msg = message();
    auto call = m_powerProfilesInterface->HoldProfile(profile, reason, applicationId);
    auto watcher = new QDBusPendingCallWatcher(call);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [msg, watcher, this] {
        watcher->deleteLater();
        QDBusPendingReply<unsigned int> reply = *watcher;
        if (reply.isError()) {
            QDBusConnection::sessionBus().send(msg.createErrorReply(watcher->error()));
        } else {
            m_holdWatcher->addWatchedService(msg.service());
            m_holdMap.insert(msg.service(), reply.value());
            QDBusConnection::sessionBus().send(msg.createReply(reply.value()));
        }
    });
    return 0; // ignored by QtDBus
}

void PowerProfile::releaseProfile(unsigned int cookie)
{
    setDelayedReply(true);
    const auto msg = message();
    auto call = m_powerProfilesInterface->ReleaseProfile(cookie);
    auto watcher = new QDBusPendingCallWatcher(call);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [msg, watcher, this] {
        watcher->deleteLater();
        if (watcher->isError()) {
            QDBusConnection::sessionBus().send(msg.createErrorReply(watcher->error()));
        } else {
            m_holdMap.remove(msg.service(), msg.arguments()[0].toUInt());
            if (m_holdMap.count(msg.service())) {
                m_holdWatcher->removeWatchedService(msg.service());
            }
            QDBusConnection::sessionBus().send(msg.createReply());
        }
    });
}

void PowerProfile::serviceUnregistered(const QString &name)
{
    const auto cookies = m_holdMap.equal_range(name);
    for (auto it = cookies.first; it != cookies.second; ++it) {
        m_powerProfilesInterface->ReleaseProfile(*it);
        m_holdMap.erase(it);
    }
    m_holdWatcher->removeWatchedService(name);
}

void PowerProfile::readProperties(const QVariantMap &properties)
{
    if (properties.contains(activeProfileProperty)) {
        m_currentProfile = properties[activeProfileProperty].toString();
        Q_EMIT currentProfileChanged(m_currentProfile);
    }

    if (properties.contains(profilesProperty)) {
        QList<QVariantMap> profiles;
        properties[profilesProperty].value<QDBusArgument>() >> profiles;
        m_profileChoices.clear();
        if (profiles.first()[QStringLiteral("Driver")] != QLatin1String("placeholder")) {
            std::transform(profiles.cbegin(), profiles.cend(), std::back_inserter(m_profileChoices), [](const QVariantMap &dict) {
                return dict[QStringLiteral("Profile")].toString();
            });
        }
        Q_EMIT profileChoicesChanged(m_profileChoices);
    }


    if (properties.contains(performanceInhibitedProperty)) {
        m_performanceInhibitedReason = properties[performanceInhibitedProperty].toString();
        Q_EMIT performanceInhibitedReasonChanged(m_performanceInhibitedReason);
    }

    if (properties.contains(performanceDegradedProperty)) {
        m_degradationReason = properties[performanceDegradedProperty].toString();
        Q_EMIT performanceDegradedReasonChanged(m_degradationReason);
    }

    if (properties.contains(profileHoldsProperty)) {
        properties[profileHoldsProperty].value<QDBusArgument>() >> m_profileHolds;
        Q_EMIT profileHoldsChanged(m_profileHolds);
    }
}

void PowerProfile::propertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated)
{
    Q_UNUSED(invalidated)
    if (interface != m_powerProfilesInterface->interface()) {
        return;
    }
    readProperties(changed);
}

#include "powerprofile.moc"
