# Builds msgpack as an ExternalProject and exposes `msgpack-c` as a static library.

ExternalProject_Add(dep-msgpack-c
    # v2.0 is now available, but contains some breaking changes. Consider upgrading.
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG cpp-1.4.2

    INSTALL_COMMAND ""
)


ExternalProject_Get_Property(dep-msgpack-c source_dir)
ExternalProject_Get_Property(dep-msgpack-c binary_dir)


# This directories don't get created until build time, but CMake will complain if
# it is not there at configure time. Create it.
file(MAKE_DIRECTORY "${source_dir}/include")


add_library(msgpack-c IMPORTED STATIC GLOBAL)
set_target_properties(msgpack-c PROPERTIES
    IMPORTED_LOCATION_DEBUG       "${binary_dir}/Debug/msgpackc${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE     "${binary_dir}/Release/msgpackc${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/include/"
)
add_dependencies(msgpack-c dep-msgpack-c)
