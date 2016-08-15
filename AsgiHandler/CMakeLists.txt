# AsgiHandler.dll - IIS Request Handler module that allows it to behave as an ASGI interface server.


add_library(AsgiHandler SHARED
    AsgiHandler.cpp
    AsgiHandler.def
)

target_link_libraries(AsgiHandler
    AsgiHandlerLib
)
