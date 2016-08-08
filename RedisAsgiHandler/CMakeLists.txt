# RedisAsgiHandler.dll - IIS Request Handler module that allows it to behave as an ASGI interface server.


add_library(RedisAsgiHandler SHARED
    RedisAsgiHandler.cpp
    RedisAsgiHandler.def
)

target_link_libraries(RedisAsgiHandler
    RedisAsgiHandlerLib
)