set(accentoutlinedecoration_SRCS
    accentoutlinedecoration.cpp
)

kconfig_add_kcfg_files(accentoutlinedecoration_SRCS accentoutlinesettings.kcfgc)

add_library(accentoutlinedecoration MODULE
    ${accentoutlinedecoration_SRCS}
    ${accentoutlinedecoration_config_PART_FORMS_HEADERS}
)

set_target_properties(accentoutlinedecoration PROPERTIES
    OUTPUT_NAME io.github.towgenik.accentoutline
)

target_include_directories(accentoutlinedecoration
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(accentoutlinedecoration
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
        KF6::ConfigCore
        KF6::ConfigGui
        KF6::CoreAddons
        KDecoration3::KDecoration
)

install(TARGETS accentoutlinedecoration
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR}
)

add_subdirectory(config)
