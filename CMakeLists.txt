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
)

set(APP_ID "org.miryugaming.grubconfig")
set(APP_EXECUTABLE "miryu-grub-config")
set(HELPER_EXECUTABLE "miryu-grub-config-helper")
set(GRUB_DEFAULT_PATH "/etc/default/grub")
set(GRUB_DROPIN_PATH "/etc/default/grub.d/99_miryu_boot.cfg")
set(GRUB_CFG_PATH "/boot/grub2/grub.cfg")
set(BLS_ENTRIES_PATH "/boot/loader/entries")
set(HELPER_INSTALL_PATH "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${HELPER_EXECUTABLE}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/polkit/org.miryugaming.grubconfig.policy.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.miryugaming.grubconfig.policy
    @ONLY
)

add_executable(${APP_EXECUTABLE}
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/grubconfig.cpp
    src/grubconfig.h
)

target_link_libraries(${APP_EXECUTABLE}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        KF6::I18n
)

target_compile_definitions(${APP_EXECUTABLE}
    PRIVATE
        APP_ID="${APP_ID}"
        HELPER_PATH="${HELPER_INSTALL_PATH}"
        GRUB_DEFAULT_PATH="${GRUB_DEFAULT_PATH}"
        BLS_ENTRIES_PATH="${BLS_ENTRIES_PATH}"
)

add_executable(${HELPER_EXECUTABLE}
    src/grubconfighelper.cpp
)

target_link_libraries(${HELPER_EXECUTABLE}
    PRIVATE
        Qt6::Core
        KF6::I18n
)

target_compile_definitions(${HELPER_EXECUTABLE}
    PRIVATE
        APP_ID="${APP_ID}"
        GRUB_DEFAULT_PATH="${GRUB_DEFAULT_PATH}"
        GRUB_DROPIN_PATH="${GRUB_DROPIN_PATH}"
        GRUB_CFG_PATH="${GRUB_CFG_PATH}"
)

install(TARGETS ${APP_EXECUTABLE} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${HELPER_EXECUTABLE} RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.miryugaming.grubconfig.policy
        DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions)
install(FILES org.miryugaming.grubconfig.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# KDE System Settings module (KCM) that auto-launches the external
# Miryu GRUB2 Boot Config GUI, mirroring the openSUSE YaST pattern:
# clicking the entry in System Settings opens the standalone app.
# Widgets-based KCMs install into the plasma/kcms/systemsettings_qwidgets
# plugin namespace so System Settings discovers them; the JSON metadata
# registers the module under the "System Administration" section.
kcoreaddons_add_plugin(kcm_miryu_grubconfig
    SOURCES kcm/kcm_miryu_grubconfig.cpp
    INSTALL_NAMESPACE "plasma/kcms/systemsettings_qwidgets"
)
target_link_libraries(kcm_miryu_grubconfig
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::WidgetsAddons
)

ki18n_install(po)
