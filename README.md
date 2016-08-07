# iis-asgi-handler

Module to allow IIS to act as an ASGI Interface Server for [Django Channels](https://github.com/andrewgodwin/channels/). Supports both HTTP and WebSockets.


## Dependencies

Relies on the following libraries, which are included as git submodules:

- `msgpack-c`
- Microsoft fork of [`hiredis`](https://github.com/Microsoft/hiredis/)
- `googletest` / `googlemock` for unit tests

Compiles on Visual Studio 2015 and uses C++ features not available in earlier versions.

There are a number of Python dependencies for the integration tests which use `pytest`. These are recorded in `IntegrationTests/requirements.txt`.


## Building

Building should be as simple as cloning the repo and submodule (`git submodule update --init`) and then building the solution.


## Installing

We aren't there yet. I don't think this will actually run with `django-channels` - not least because the ASGI spec has probably moved on since this was first written.


## Tests

There are two sets of tests:

- Unit tests, in `RedisAsgiHandlerTests/`. These use `googletest` and `googlemock`.
- Integration tests, in `IntegrationTests/`. These use `pytest` and install the module into IIS before each test.

### Integration Tests

These are written with `pytest` and contain some useful fixtures for:
 - Installing/uninstalling the module into IIS for each test.
 - Capturing the ETW log output.
 - Creating a new Channels layer for each test.

They were written using Python Tools for Visual Studio. The requirements are in `requirements.txt` as usual, but I'm not sure how that works when cloning the repository... you will either have to convince Visual Studio to create a venv with the `requirements.txt` or run the tests outside Visual Studio.
