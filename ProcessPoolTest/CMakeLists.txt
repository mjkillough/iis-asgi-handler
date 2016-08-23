# ProcessPoolTest.exe - Unit tests for ProcessPoolLib.lib


set(SOURCES
    main.cpp

    test_ProcessPool.cpp
)


set(HEADERS
)


add_executable(ProcessPoolTest
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(ProcessPoolTest
    ProcessPoolLib
    googletest
    googlemock
)


source_group("Header Files\\Mocks" REGULAR_EXPRESSION /mock_[^/]*.h$)
source_group("Source Files\\Tests" REGULAR_EXPRESSION /test_[^/]*.cpp$)


add_test(NAME ProcessPoolTest
    COMMAND $<TARGET_FILE:ProcessPoolTest>
)
