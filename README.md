# iis-asgi-handler — [![Build status](https://ci.appveyor.com/api/projects/status/yuaoo10qojr5825j/branch/master?svg=true)](https://ci.appveyor.com/project/mjkillough/iis-asgi-handler/branch/master)

Module to allow IIS to act as an ASGI Interface Server for [Django Channels](https://github.com/andrewgodwin/channels/). Supports both HTTP and WebSockets.


## Building

Requires:
- Visual Studio 2015
- CMake 2.8+

To build:

- `mkdir build && cd build`
- Then either:
    - x86: `cmake .. -G "Visual Studio 14 2015"`
    - x64: `cmake .. -G "Visual Studio 14 2015 Win64"`
- `cmake --build . --configuration Debug`
    - Or open the `.sln` file it creates.


## Installing

It isn't ready for people to install and use. If you're really keen, you can look at the code in `IntegrationTests/fixtures/` for an idea of how to install it.

The module can not currently be run on IIS 7.5 and it requires the Web Sockets module to be installed. Eventually we will support IIS 7.5 (without Web Socket support).


## Tests

There are two sets of tests:

- Unit tests, in `RedisAsgiHandlerTests/`. These use `googletest` and `googlemock`.
- Integration tests, in `IntegrationTests/`. These use `pytest` and install the module into IIS before each test.

To run the integration tests you will need the following installed:
- [redis](https://github.com/MSOpenTech/Redis) (or [redis-64](https://chocolatey.org/packages/redis-64) chocolatey package).
- IIS 8+ with the Web Sockets module installed.

To run all the tests: `ctest -C Debug --output-on-failure`. Append `-R AsgiHandler` to run just the unit tests, or `-R Integration` to run just the integration tests.


## Dependencies

Relies on the following libraries, which are downloaded and built automatically:

- `msgpack-c`
- Microsoft fork of [`hiredis`](https://github.com/Microsoft/hiredis/)
- `googletest` / `googlemock` for unit tests

There are a number of Python dependencies recorded in `IntegrationTests/requirements.txt`. These are automatically installed into a venv when the project is built.
