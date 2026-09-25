set(accentoutlineconfig_SRCS
    accentoutlineconfigwidget.cpp
)

kcoreaddons_add_plugin(kcm_accentoutline
    SOURCES
        kcm_accentoutline.cpp
        ${accentoutlineconfig_SRCS}
    INSTALL_NAMESPACE "${KDECORATION_KCM_PLUGIN_DIR}"
)

kconfig_add_kcfg_files(kcm_accentoutline ../accentoutlinesettings.kcfgc)

target_include_directories(kcm_accentoutline
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}/..
)

target_link_libraries(kcm_accentoutline
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
        KF6::ConfigCore
        KF6::ConfigGui
        KF6::CoreAddons
        KF6::I18n
        KF6::KCMUtils
)

kcmutils_generate_desktop_file(kcm_accentoutline)
