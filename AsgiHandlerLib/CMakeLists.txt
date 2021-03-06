# AsgiHandlerLib.lib - For use in AsgiHandler.dll, but compiled as a lib so that it can also be used by tests.


set(SOURCES
    HttpModule.cpp
    HttpModuleFactory.cpp

    RedisChannelLayer.cpp
    ResponsePump.cpp

    RequestHandler.cpp
    HttpRequestHandler.cpp
    HttpRequestHandlerSteps.cpp

    WsRequestHandler.cpp
    WsRequestHandlerSteps.cpp
    WsReader.cpp
    WsWriter.cpp
)


set(HEADERS
    HttpModule.h
    HttpModuleFactory.h

    ChannelLayer.h
    ResponsePump.h

    ScopedRedisReply.h
    RedisChannelLayer.h

    RequestHandler.h
    HttpRequestHandler.h
    HttpRequestHandlerSteps.h

    WsRequestHandler.h
    WsRequestHandlerSteps.h
    WsReader.h
    WsWriter.h

    AsgiMsg.h
    AsgiHttpRequestMsg.h
    AsgiHttpResponseMsg.h
    AsgiWsConnectMsg.h
    AsgiWsReceiveMsg.h
    AsgiWsSendMsg.h
)


add_library(AsgiHandlerLib STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(AsgiHandlerLib
    SharedUtils
    hiredis
    hiredis-interop
    msgpack-c
)

target_include_directories(AsgiHandlerLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:.>
)


source_group("Source Files\\Http" REGULAR_EXPRESSION /Http[^/]*.cpp$)
source_group("Header Files\\Http" REGULAR_EXPRESSION /Http[^/]*.h$)
source_group("Source Files\\Ws"   REGULAR_EXPRESSION /Ws[^/]*.cpp$)
source_group("Header Files\\Ws"   REGULAR_EXPRESSION /Ws[^/]*.h$)
source_group("Header Files\\Asgi" REGULAR_EXPRESSION /Asgi[^/]*$)
