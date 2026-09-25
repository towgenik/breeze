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

# kcmshell6 only searches the "plasma/kcms*" namespaces, so the desktop entry has
# to address this module the same way kwin's decoration KCM does: by its
# namespace-relative plugin id ("org.kde.kdecoration3.kcm/kcm_accentoutline").
# kcmutils_generate_desktop_file() would emit the bare "kcm_accentoutline" id,
# which kcmshell6 cannot resolve, and would mark the entry NoDisplay.
configure_file(kcm_accentoutline.desktop.in kcm_accentoutline.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_accentoutline.desktop DESTINATION ${KDE_INSTALL_APPDIR})
