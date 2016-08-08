# Builds googletest as an ExternalProject and exposes `googletest` and `googlemock` as static libraries.


ExternalProject_Add(dep-googletest
    # Unfortunately, the last released version is quite old and does not have
    # support for VS 2015. This commit is from master on 2016/08/07 - not a
    # lot of intelligence went into picking it.
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG ec44c6c1675c25b9827aacd08c02433cccde7780

    # Don't install.
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(dep-googletest source_dir)
ExternalProject_Get_Property(dep-googletest binary_dir)


# deps-googletest doesn't create these directories until it's built, yet CMake will
# complain at configure time if they don't exist. Create them to silence CMake.
file(MAKE_DIRECTORY "${source_dir}/googletest/include")
file(MAKE_DIRECTORY "${source_dir}/googlemock/include")


add_library(googletest IMPORTED STATIC GLOBAL)
set_target_properties(googletest PROPERTIES
    IMPORTED_LOCATION_DEBUG       "${binary_dir}/googlemock/gtest/Debug/gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE     "${binary_dir}/googlemock/gtest/Release/gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/googletest/include"
)
add_dependencies(googletest dep-googletest)


add_library(googlemock IMPORTED STATIC GLOBAL)
set_target_properties(googlemock PROPERTIES
    IMPORTED_LOCATION_DEBUG       "${binary_dir}/googlemock/Debug/gmock${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE     "${binary_dir}/googlemock/Release/gmock${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/googlemock/include"
)
add_dependencies(googlemock dep-googletest)
