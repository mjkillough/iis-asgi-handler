# Builds hiredis as an ExternalProject and exposes `hiredis` and `hiredis-interop` as static libraries.
# Both must be included by dependent targets.


set(solution_dir msvs/vs-solutions/vs2015)

ExternalProject_Add(dep-hiredis
    # We need to use the Microsoft fork of hiredis. It seems to be kept up-to-date
    # with redis/hiredis, but does not seem to make releases. Use master as of 2016/08/07.
    GIT_REPOSITORY https://github.com/Microsoft/hiredis.git
    GIT_TAG 05362104fa8d42a5ad64c042a79cbefc8fcfec33

    # The project is checked out into a intermediate directory, so this isn't truly
    # building in _our_ source. It's necessary because hiredis doesn't use CMake.
    BUILD_IN_SOURCE 1

    CONFIGURE_COMMAND ""
    # Always build both configurations. CMake targets will consume the right one.
    BUILD_COMMAND msbuild "/p:Configuration=Debug" "/p:Platform=${TARGET_PLATFORM}" "${solution_dir}/hiredis_win.sln"
    COMMAND       msbuild "/p:Configuration=Release" "/p:Platform=${TARGET_PLATFORM}" "${solution_dir}/hiredis_win.sln"
    INSTALL_COMMAND ""

    LOG_BUILD 1
)

ExternalProject_Get_Property(dep-hiredis source_dir)
set(binary_dir "${source_dir}/${solution_dir}/bin/${TARGET_PLATFORM}")


add_library(hiredis IMPORTED STATIC GLOBAL)
set_target_properties(hiredis PROPERTIES
    IMPORTED_LOCATION_DEBUG       "${binary_dir}/Debug/hiredis${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE     "${binary_dir}/Release/hiredis${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/"
)
add_dependencies(hiredis dep-hiredis)


add_library(hiredis-interop IMPORTED STATIC GLOBAL)
set_target_properties(hiredis-interop PROPERTIES
    IMPORTED_LOCATION_DEBUG       "${binary_dir}/Debug/win32_interop${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE     "${binary_dir}/Release/win32_interop${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/"
)
add_dependencies(hiredis-interop dep-hiredis)
