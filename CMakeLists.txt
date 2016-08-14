cmake_minimum_required(VERSION 2.6)

project(IisAsgiHandler)

enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS On)


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} /MTd")


# Check whether we're building for x64 or x86
if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(TARGET_PLATFORM "x64")
else()
    set(TARGET_PLATFORM "x86")
endif()


add_subdirectory(dependencies)
add_subdirectory(cli)
add_subdirectory(RedisAsgiHandlerLib)
add_subdirectory(RedisAsgiHandlerTest)
add_subdirectory(RedisAsgiHandler)
