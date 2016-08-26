# AsgiHandlerTest.exe - Unit tests for AsgiHandlerLib.lib


set(SOURCES
    main.cpp

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
    mock_ChannelLayer.h
)


add_executable(AsgiHandlerTest
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(AsgiHandlerTest
    AsgiHandlerLib
    googletest
    googlemock
)


source_group("Header Files\\Mocks" REGULAR_EXPRESSION /mock_[^/]*.h$)
source_group("Source Files\\Tests" REGULAR_EXPRESSION /test_[^/]*.cpp$)
source_group("Source Files\\Tests\\Steps" REGULAR_EXPRESSION /test_[^/]*Step.cpp$)


add_test(NAME AsgiHandlerTest
    COMMAND $<TARGET_FILE:AsgiHandlerTest>
)
