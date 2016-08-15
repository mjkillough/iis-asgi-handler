#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function, absolute_import

import os
import logging
import subprocess
import collections
import concurrent.futures

import pytest
import asgi_redis
import requests_futures.sessions

from .asgi import asgi
from .django_worker import django_worker
from .etw import *


logger = logging.getLogger(__name__)


MODULE_NAME = 'AsgiHandler'
DEFAULT_DLL_PATH = os.path.join(
    os.path.dirname(__file__), '..', '..', 'build', 'AsgiHandler', 'Debug', 'AsgiHandler.dll'
)


def pytest_addoption(parser):
    parser.addoption(
        '--asgi-handler-dll', action='store', default=DEFAULT_DLL_PATH,
        help='Path to the AsgiHandler.dll that is to be tested'
    )
@pytest.fixture
def asgi_handler_dll(request):
    return request.config.getoption('--asgi-handler-dll')


# Taken from pytest docs - makes report available in fixture.
@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # execute all other hooks to obtain the report object
    outcome = yield
    if call.when == 'call':
        rep = outcome.get_result()
        setattr(item, "_report", rep)


def appcmd(*args):
    path = os.path.join('C:\\', 'Windows', 'System32', 'inetsrv', 'appcmd.exe')
    cmd = [path] + list(args)
    logger.debug('Calling: %s', ' '.join(cmd))
    output = subprocess.check_output(cmd)
    logger.debug('Returned: %s', output)
    return output

def uninstall_module():
    appcmd('uninstall', 'module', MODULE_NAME)

def install_module(path):
    appcmd('install', 'module', '/name:%s' % MODULE_NAME, '/image:%s' % path)


@pytest.yield_fixture
def install_iis_module(asgi_handler_dll, etw_consumer):
    try:
        install_module(asgi_handler_dll)
        yield
    finally:
        # Allow errors to propogate, as they could affect the ability of other tests
        # to re-install the module.
        uninstall_module()

_Site = collections.namedtuple('_Site', ('url', 'https_url', 'ws_url', 'static_path'))
@pytest.yield_fixture
def site(tmpdir, install_iis_module):
    pool_name = 'asgi-test-pool'
    site_name = 'asgi-test-site'
    http_port = 90
    https_port = 91
    try:
        # Create a web.config which sends most requests through
        # our handler. We create a subdirectory that serves static files
        # so that we can test our handler doesn't get in the way
        # of others.
        webconfig = tmpdir.join('web.config')
        webconfig.write("""
            <configuration>
                <system.webServer>
                    <handlers>
                        <clear />
                        <add
                            name="StaticFile"
                            path="static.html"
                            verb="*"
                            modules="StaticFileModule"
                            resourceType="File"
                        />
                        <add
                            name="AsgiHandler"
                            path="*"
                            verb="*"
                            modules="AsgiHandler"
                        />
                    </handlers>
                </system.webServer>
            </configuration>
        """)
        staticfile = tmpdir.join('static.html')
        staticfile.write('Hello, world!')
        # Ensure IIS can read the directory. Use icacls to avoid introducing
        # pywin32 dependency.
        subprocess.check_call(['icacls', str(tmpdir), '/grant', 'Users:R'])
        subprocess.check_call(['icacls', str(webconfig), '/grant', 'Users:R'])
        subprocess.check_call(['icacls', str(staticfile), '/grant', 'Users:R'])
        # Add the site with its own app pool. Failing tests can cause the
        # pool to stop, so we create a new one for each test.
        appcmd('add', 'apppool', '/name:' + pool_name)
        appcmd('add', 'site',
            '/name:' + site_name,
            '/bindings:http://*:%i,https://*:%i' % (http_port, https_port),
            '/physicalPath:' + str(tmpdir),
        )
        appcmd('set', 'app', site_name + '/', '/applicationPool:' + pool_name)
        appcmd('unlock', 'config', '-section:system.webServer/handlers')
        yield _Site(
            'http://localhost:%i' % http_port,
            'https://localhost:%i' % https_port,
            'ws://localhost:%i' % http_port,
            '/static.html'
        )
    finally:
        appcmd('delete', 'apppool', pool_name)
        appcmd('delete', 'site', site_name)



class _ThreadPoolExecutor(concurrent.futures.ThreadPoolExecutor):
    """An executor that remembers its futures, so that they can be inspected on test failure."""
    def __init__(self, *args, **kwargs):
        super(_ThreadPoolExecutor, self).__init__(*args, **kwargs)
        self.futures = []

    def submit(self, fn, *args, **kwargs):
        future = super(_ThreadPoolExecutor, self).submit(fn, *args, **kwargs)
        self.futures.append(future)
        return future

@pytest.yield_fixture
def session(request):
    # IIS on non-server OS only supports 10 concurrent connections, so there is no point
    # starting many more workers.
    executor = _ThreadPoolExecutor(max_workers=10)
    try:
        yield requests_futures.sessions.FuturesSession(executor)
    finally:
        # If the test failed, append a summary of how many outstanding tasks
        # we had. This can be useful to identify hung servers and when IIS has
        # returned a response when we weren't expecting one (this usually indicates
        # an error like a stopped AppPool).
        if hasattr(request.node, '_report'):
            if not request.node._report.passed:
                request.node._report.longrepr.addsection(
                    'Summary of requests-futures futures',
                    '\r\n'.join(repr(future) for future in executor.futures)
                )
                for future in executor.futures:
                    if future.done():
                        response = future.result()
                        request.node._report.longrepr.addsection(
                            'Response for request: %s %s' % (response.request.method, response.request.url),
                            (
                                'Status: %s\r\n'
                                'Headers: %s\r\n'
                                'Body:\r\n%s'
                                '\r\n\r\n---\r\n\r\n'
                            ) % (response.status_code, response.headers, response.text)
                        )
        executor.shutdown(wait=False)
