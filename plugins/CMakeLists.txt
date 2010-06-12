add_subdirectory(lib)

OPTION(ENABLE_PLUGIN_COINCOIN "Enable coincoin plugin build" ON)
IF(ENABLE_PLUGIN_COINCOIN)
    add_subdirectory(coincoin)
ENDIF(ENABLE_PLUGIN_COINCOIN)

OPTION(ENABLE_PLUGIN_GAYATTITUDE "Enable coincoin plugin build" ON)
IF(ENABLE_PLUGIN_GAYATTITUDE)
    add_subdirectory(gayattitude)
ENDIF(ENABLE_PLUGIN_GAYATTITUDE)
