add_definitions(-DTRANSLATION_DOMAIN=\"powerdevil\" -DQT_USE_QSTRINGBUILDER)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(actions)

# target no.1 - powerdevil core library
set(powerdevilcore_SRCS
    powerdevil_debug.cpp
    powerdevilaction.cpp
    powerdevilactionpool.cpp
    powerdevilbackendinterface.cpp
    powerdevilcore.cpp
    powerdevilpolicyagent.cpp
    powerdevilprofilegenerator.cpp
    powerdevilbrightnesslogic.cpp
    powerdevilscreenbrightnesslogic.cpp
    powerdevilkeyboardbrightnesslogic.cpp
    powerdevilpowermanagement.cpp
    brightnessosdwidget.cpp
    kwinkscreenhelpereffect.cpp
)

add_library(powerdevil_log_static STATIC)
set_property(TARGET powerdevil_log_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(powerdevil_log_static Qt::Core)
ecm_qt_declare_logging_category(powerdevil_log_static
   HEADER powerdevil_debug.h
   IDENTIFIER POWERDEVIL
   CATEGORY_NAME org.kde.powerdevil
   DEFAULT_SEVERITY Warning
   DESCRIPTION "Powerdevil" EXPORT POWERDEVIL)

kconfig_add_kcfg_files(powerdevilcore_SRCS ../PowerDevilSettings.kcfgc)

# Action DBus Adaptors

qt_add_dbus_interface(powerdevilcore_SRCS org.freedesktop.ScreenSaver.xml screenlocker_interface)

add_library(powerdevilcore SHARED ${powerdevilcore_SRCS} powerdevil_debug.cpp)
set_target_properties(powerdevilcore PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 2)

# not exported, so just make the deps public
target_link_libraries(powerdevilcore
    KF5::Activities
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::DBusAddons
    KF5::I18n
    KF5::Solid
    KF5::IdleTime
    KF5::GlobalAccel
    KF5::Screen
    KF5::Service
    KF5::WidgetsAddons
    KF5::Notifications
    KF5::XmlGui
    KF5::WaylandClient
    KF5::Kirigami2
    PW::KWorkspace
    XCB::DPMS
    powerdevil_log_static
)

if(HAVE_WIRELESS_SUPPORT)
    target_link_libraries(powerdevilcore
        KF5::NetworkManagerQt
        KF5::BluezQt
    )
endif()

if (XCB_FOUND) # kwin kscreen helper effect
    target_link_libraries(powerdevilcore XCB::XCB)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(powerdevilcore Qt5::X11Extras)
    else()
       target_link_libraries(powerdevilcore Qt::GuiPrivate)
    endif()
endif ()

# target no.2 - powerdevil kded module
set(powerdevil_SRCS
    powerdevil_debug.cpp
    powerdevilapp.cpp
    powerdevilfdoconnector.cpp
)

# DBus Adaptors
qt_add_dbus_adaptor(powerdevil_SRCS org.kde.Solid.PowerManagement.xml powerdevilcore.h PowerDevil::Core)
qt_add_dbus_adaptor(powerdevil_SRCS org.kde.Solid.PowerManagement.PolicyAgent.xml
                                         powerdevilpolicyagent.h PowerDevil::PolicyAgent
                                         powermanagementpolicyagentadaptor PowerManagementPolicyAgentAdaptor)

qt_add_dbus_adaptor(powerdevil_SRCS org.freedesktop.PowerManagement.xml powerdevilfdoconnector.h PowerDevil::FdoConnector powermanagementfdoadaptor PowerManagementFdoAdaptor)
qt_add_dbus_adaptor(powerdevil_SRCS org.freedesktop.PowerManagement.Inhibit.xml powerdevilfdoconnector.h PowerDevil::FdoConnector powermanagementinhibitadaptor PowerManagementInhibitAdaptor)

# KAuth charge threshold helper
add_executable(chargethresholdhelper chargethresholdhelper.cpp powerdevil_debug.cpp ${chargethresholdhelper_mocs})
target_link_libraries(chargethresholdhelper Qt::Core KF5::AuthCore)
install(TARGETS chargethresholdhelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_helper_files(chargethresholdhelper org.kde.powerdevil.chargethresholdhelper root)
kauth_install_actions(org.kde.powerdevil.chargethresholdhelper chargethreshold_helper_actions.actions)

# Backends
add_subdirectory(backends)

add_executable(powerdevil ${powerdevil_SRCS})
set_target_properties(powerdevil PROPERTIES OUTPUT_NAME org_kde_powerdevil)
configure_file(powerdevil.desktop.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/powerdevil.desktop)

target_link_libraries(powerdevil
    KF5::Crash
    KF5::ConfigCore
    KF5::CoreAddons
    KF5::I18n
    KF5::DBusAddons
    PW::KWorkspace
    powerdevilcore
)
if (XCB_FOUND)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(powerdevil Qt5::X11Extras)
    else()
        target_link_libraries(powerdevil Qt::GuiPrivate)
    endif()
endif ()

target_link_libraries(powerdevil ${UDEV_LIBS})

install(TARGETS powerdevil DESTINATION ${KDE_INSTALL_FULL_LIBEXECDIR})
if (HAVE_LIBCAP)
    install(
    CODE "execute_process(
            COMMAND
                ${SETCAP_EXECUTABLE}
                CAP_WAKE_ALARM=+ep
                \$ENV{DESTDIR}${KDE_INSTALL_FULL_LIBEXECDIR}/org_kde_powerdevil)"
    )
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/powerdevil.desktop
        DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install(TARGETS powerdevilcore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# target no.3 - powerdevil ui library
set(powerdevilui_SRCS
    powerdevilactionconfig.cpp
)

add_library(powerdevilui ${powerdevilui_SRCS})
set_target_properties(powerdevilui PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

# not exported, so just make the deps public
target_link_libraries(powerdevilui
    PUBLIC
        Qt::Widgets
        KF5::ConfigCore
)

set(HAVE_XCB ${XCB_FOUND})
configure_file(config-powerdevil.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-powerdevil.h )

install(TARGETS powerdevilui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
ecm_install_configured_files(INPUT plasma-powerdevil.service.in @ONLY DESTINATION
        ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
