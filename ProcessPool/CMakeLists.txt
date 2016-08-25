# ProcessPool.dll - IIS module that creates a pool of processes with the same lifetime as the application pool.


add_library(ProcessPool SHARED
    ProcessPool.cpp
    ProcessPool.def

    process-pool-iis-schema.xml
)

target_link_libraries(ProcessPool
    ProcessPoolLib
)
