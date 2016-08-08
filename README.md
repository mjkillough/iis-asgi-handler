# iis-asgi-handler

Module to allow IIS to act as an ASGI Interface Server for [Django Channels](https://github.com/andrewgodwin/channels/). Supports both HTTP and WebSockets.


## Dependencies

Relies on the following libraries, which are downloaded and built automatically:

- `msgpack-c`
- Microsoft fork of [`hiredis`](https://github.com/Microsoft/hiredis/)
- `googletest` / `googlemock` for unit tests

Compiles on Visual Studio 2015 and uses C++ features not available in earlier versions.

There are a number of Python dependencies for the integration tests which use `pytest`. These are recorded in `IntegrationTests/requirements.txt`.


## Building

Requires CMake.

To build:

- `mkdir out && cd out`
- Then either:
    - x86: `cmake .. -G "Visual Studio 14 2015"`
    - x64: `cmake .. -G "Visual Studio 14 2015 Win64"`
- Open the `.sln` file it creates and build.
    - Or from the command line: `cmake --build . --config Release`


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

They are not yet integrated into the CMake scripts.
