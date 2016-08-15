# IntegrationTests - Installs compiled module into IIS and runs tests against it.

# Find appropriate Python and then find its copy of virtualenv. This is
# very Windows specific, but we only need to run on Windows. Unfortunately,
# we can't rely on virtualenv being in the path.
find_package(PythonInterp 2 EXACT REQUIRED)
get_filename_component(PYTHON_DIRECTORY ${PYTHON_EXECUTABLE} DIRECTORY)
set(PYTHON_SCRIPTS_DIRECTORY ${PYTHON_DIRECTORY}/Scripts)
set(VIRTUALENV ${PYTHON_SCRIPTS_DIRECTORY}/virtualenv.exe)
if(NOT EXISTS ${VIRTUALENV})
    message(FATAL_ERROR "Could not find `virtualenv` at: ${VIRTUALENV}")
endif()

# Generate the virtualenv and ensure it's up to date.
add_custom_command(
    OUTPUT venv
    COMMAND ${VIRTUALENV} venv
)
add_custom_command(
    OUTPUT venv.stamp
    DEPENDS venv requirements.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt requirements.txt
    COMMAND ./venv/Scripts/pip.exe install -r requirements.txt --upgrade
)

# Build command line to run py.test. We disable the cachedir as it doesn't seem
# possible to cause to happen out of the source tree.
set(PYTEST
    ${CMAKE_CURRENT_BINARY_DIR}/venv/Scripts/python.exe
    ${CMAKE_CURRENT_BINARY_DIR}/venv/Scripts/py.test.exe
    -p no:cacheprovider
)


set(TESTS
    test_config.py
    test_asgi_http.py
    test_asgi_ws.py
    test_django_http.py
)

set(FIXTURES
    conftest.py

    fixtures/__init__.py
    fixtures/asgi.py
    fixtures/etw.py
    fixtures/django_worker.py
)

add_custom_target(IntegrationTests ALL
    SOURCES ${TESTS} ${FIXTURES} requirements.txt
    DEPENDS venv.stamp
)

source_group(fixtures REGULAR_EXPRESSION fixtures/.*)


add_test(NAME IntegrationTests
    COMMAND ${PYTEST} --asgi-handler-dll $<TARGET_FILE:AsgiHandler> ${TESTS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
