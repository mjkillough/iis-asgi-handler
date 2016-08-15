# IntegrationTests - Installs compiled module into IIS and runs tests against it.


set(TESTS
    test.py
    test_config.py
    test_http.py
    test_ws.py
)

set(FIXTURES
    conftest.py

    fixtures/__init__.py
    fixtures/asgi.py
    fixtures/etw.py
)


add_custom_target(IntegrationTests
    SOURCES ${TESTS} ${FIXTURES} requirements.txt
)

source_group(fixtures REGULAR_EXPRESSION fixtures/.*)
