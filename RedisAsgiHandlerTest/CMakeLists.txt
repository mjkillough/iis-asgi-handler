# RedisAsgiHandlerTest.exe - Unit tests for RedisAsgiHandler.{dll,lib}


set(SOURCES
    main.cpp

    test_Logger.cpp
    test_ReadBodyStep.cpp
    test_WaitForResponseStep.cpp
    test_FlushResponseStep.cpp
    test_RedisChannelLayer.cpp
    test_WriteResponseStep.cpp
    test_SendToApplicationStep.cpp
)


set(HEADERS
    mock_httpserv.h
    mock_ResponsePump.h
    mock_HttpRequestHandler.h
    mock_RedisChannelLayer.h
    mock_Logger.h
)


add_executable(RedisAsgiHandlerTest
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(RedisAsgiHandlerTest
    RedisAsgiHandlerLib
    googletest
    googlemock
)


source_group(Mocks REGULAR_EXPRESSION "^mock_.*$")
source_group(Tests REGULAR_EXPRESSION "^tests_.*$")


add_test(NAME RedisAsgiHandlerTest
    COMMAND $<TARGET_FILE:RedisAsgiHandlerTest>
)
