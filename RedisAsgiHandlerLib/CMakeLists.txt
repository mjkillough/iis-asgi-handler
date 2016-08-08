# RedisAsgiHandler.lib - For use in RedisAsgiHandler.dll, but compiled as a lib so that it can also be used by tests.


set(SOURCES
    HttpModule.cpp
    HttpModuleFactory.cpp

    Logger.cpp
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

    ScopedRedisReply.h
    Logger.h
    RedisChannelLayer.h
    ResponsePump.h

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


add_library(RedisAsgiHandlerLib STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(RedisAsgiHandlerLib
    hiredis
    hiredis-interop
    msgpack-c
)

target_include_directories(RedisAsgiHandlerLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:.>
)


source_group(Http REGULAR_EXPRESSION "^Http.*")
source_group(Ws   REGULAR_EXPRESSION "^Ws.*")
source_group(Asgi REGULAR_EXPRESSION "^Asgi.*")
