# SharedUtilsTest.exe - Unit tests for SharedUtils.lib


set(SOURCES
    main.cpp

    test_Logger.cpp
)


set(HEADERS
)


add_executable(SharedUtilsTest
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(SharedUtilsTest
    AsgiHandlerLib
    googletest
    googlemock
)


source_group("Header Files\\Mocks" REGULAR_EXPRESSION /mock_[^/]*.h$)
source_group("Source Files\\Tests" REGULAR_EXPRESSION /test_[^/]*.cpp$)


add_test(NAME SharedUtilsTest
    COMMAND $<TARGET_FILE:SharedUtilsTest>
)
