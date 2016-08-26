# SharedUtils.lib - Static library of shared utilities for ProcessPool.lib and AsgiHandler.dll


set(SOURCES
    Logger.cpp
)


set(HEADERS
    Logger.h
    ScopedResources.h

    # This is here because it's used by multiple *Test projects.
    mock_Logger.h
)


add_library(SharedUtils STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(SharedUtils
)

target_include_directories(SharedUtils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:.>
)

