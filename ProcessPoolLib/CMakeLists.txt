# ProcessPoolLib.lib - For use in ProcessPool.dll, but compiled as a lib so that it can also be used by tests.

set(SOURCES
    ProcessPool.cpp
    JobObject.cpp
)


set(HEADERS
    ProcessPool.h
    JobObject.h

    ScopedHandle.h
)


add_library(ProcessPoolLib STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(AsgiHandlerLib
)

target_include_directories(ProcessPoolLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:.>
)

