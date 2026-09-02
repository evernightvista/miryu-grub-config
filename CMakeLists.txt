cmake_minimum_required(VERSION 3.16)

project(miryu-grub-config
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
# KDECompilerSettings is intentionally NOT included: it enables
# QT_NO_KEYWORDS / QT_NO_CAST_FROM_ASCII / QT_NO_FOREACH and the KDE
# strict-warning set, which the existing app sources were not written
# against. The KCM uses Q_SLOTS / QStringLiteral / i18nd, which compile
# cleanly with or without it; KDEInstallDirs above still provides the
# plugin install namespace the KCM needs.

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    I18n
    KCMUtils
    WidgetsAddons
    Auth
)

set(APP_ID "org.miryugaming.grubconfig")
set(APP_EXECUTABLE "miryu-grub-config")
set(KAUTH_HELPER_EXECUTABLE "miryu-grub-config-kauth-helper")
set(GRUB_DEFAULT_PATH "/etc/default/grub")
set(GRUB_DROPIN_PATH "/etc/default/grub.d/99_miryu_boot.cfg")
set(GRUB_CFG_PATH "/boot/grub2/grub.cfg")
set(BLS_ENTRIES_PATH "/boot/loader/entries")
set(HELPER_INSTALL_PATH "${CMAKE_INSTALL_FULL_LIBEXECDIR}/kauth/${KAUTH_HELPER_EXECUTABLE}")

# ── Standalone application ──

add_executable(${APP_EXECUTABLE}
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/grubconfigwidget.cpp
    src/grubconfigwidget.h
    src/grubconfig.cpp
    src/grubconfig.h
)

target_link_libraries(${APP_EXECUTABLE}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        KF6::I18n
        KF6::AuthCore
)

target_compile_definitions(${APP_EXECUTABLE}
    PRIVATE
        APP_ID="${APP_ID}"
        GRUB_DEFAULT_PATH="${GRUB_DEFAULT_PATH}"
        BLS_ENTRIES_PATH="${BLS_ENTRIES_PATH}"
)

install(TARGETS ${APP_EXECUTABLE} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES org.miryugaming.grubconfig.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# ── KAuth helper (runs as root via KAuth/polkit) ──
#
# The helper receives the generated GRUB2 configuration content through
# KAuth's QVariantMap arguments (no temp files, no pkexec CLI). It merges
# the settings into /etc/default/grub, updates BLS entries via grubby, and
# regenerates grub.cfg. The call from the UI is fully asynchronous through
# KAuth::ExecuteJob so the interface never freezes while waiting for
# authentication or grub2-mkconfig.

add_executable(${KAUTH_HELPER_EXECUTABLE}
    src/grubconfigkauthhelper.cpp
    src/grubconfigkauthhelper.h
)

target_link_libraries(${KAUTH_HELPER_EXECUTABLE}
    PRIVATE
        Qt6::Core
        KF6::I18n
        KF6::AuthCore
)

target_compile_definitions(${KAUTH_HELPER_EXECUTABLE}
    PRIVATE
        GRUB_DEFAULT_PATH="${GRUB_DEFAULT_PATH}"
        GRUB_DROPIN_PATH="${GRUB_DROPIN_PATH}"
        GRUB_CFG_PATH="${GRUB_CFG_PATH}"
)

install(TARGETS ${KAUTH_HELPER_EXECUTABLE} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/kauth)

# Manually generate and install D-Bus service file, D-Bus config, and
# polkit policy instead of relying on kauth_install_helper_files /
# kauth_install_actions. This ensures the Exec path in the D-Bus service
# file is always correct regardless of whether KAUTH_HELPER_INSTALL_DIR is
# defined by the ECM version on the build host.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus/org.miryugaming.grubconfig.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.miryugaming.grubconfig.service
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/polkit/org.miryugaming.grubconfig.policy.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.miryugaming.grubconfig.policy
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.miryugaming.grubconfig.service
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
install(FILES dbus/org.miryugaming.grubconfig.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.miryugaming.grubconfig.policy
        DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions)

# ── KDE System Settings module (KCM) ──
#
# Embeds the full Miryu GRUB2 Boot Config GUI directly. Reuses
# GrubConfigWidget + GrubConfig from src/ so the integrated KCM and the
# standalone application share one implementation of the boot-menu/kernel-
# parameter editing and the KAuth save flow. Widgets-based KCMs install
# into the plasma/kcms/systemsettings_qwidgets plugin namespace so System
# Settings discovers them; the JSON metadata registers the module under
# the "System Administration" section.

kcoreaddons_add_plugin(kcm_miryu_grubconfig
    SOURCES
        kcm/kcm_miryu_grubconfig.cpp
        src/grubconfigwidget.cpp
        src/grubconfigwidget.h
        src/grubconfig.cpp
        src/grubconfig.h
    INSTALL_NAMESPACE "plasma/kcms/systemsettings_qwidgets"
)
target_include_directories(kcm_miryu_grubconfig PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(kcm_miryu_grubconfig
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::WidgetsAddons
    KF6::AuthCore
)
target_compile_definitions(kcm_miryu_grubconfig
    PRIVATE
        GRUB_DEFAULT_PATH="${GRUB_DEFAULT_PATH}"
        BLS_ENTRIES_PATH="${BLS_ENTRIES_PATH}"
)

ki18n_install(po)
