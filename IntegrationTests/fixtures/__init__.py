#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function, absolute_import

import os
import shutil
import logging
import ctypes
import subprocess
import collections
import tempfile
import concurrent.futures

import pytest
import asgi_redis
import requests_futures.sessions

from .asgi import asgi
from .django_worker import django_worker
from .etw import *


logger = logging.getLogger(__name__)


BITNESS_64BIT = 'x64'
ASGI_MODULE_NAME = 'AsgiHandler'
POOL_MODULE_NAME = 'ProcessPool'
DEFAULT_ASGI_DLL_PATH = os.path.join(
    os.path.dirname(__file__), '..', '..', 'build', 'AsgiHandler', 'Debug', 'AsgiHandler.dll'
)
DEFAULT_POOL_DLL_PATH = os.path.join(
    os.path.dirname(__file__), '..', '..', 'build', 'ProcessPool', 'Debug', 'ProcessPool.dll'
)
DEFAULT_POOL_SCHEMA_PATH = os.path.join(
    # NOTE: Source directory.
    os.path.dirname(__file__), '..', '..', 'ProcessPool', 'process-pool-iis-schema.xml'
)

IIS_SCHEMA_PATH = os.path.expandvars('%WinDir%\\System32\\inetsrv\\config\\schema\\')
POOL_SCHEMA_INSTALL_PATH = os.path.join(IIS_SCHEMA_PATH, 'process-pool.xml')


def pytest_addoption(parser):
    parser.addoption(
        '--asgi-handler-dll', action='store', default=DEFAULT_ASGI_DLL_PATH,
        help='Path to the AsgiHandler.dll that is to be tested'
    )
    parser.addoption(
        '--process-pool-dll', action='store', default=DEFAULT_POOL_DLL_PATH,
        help='Path to the ProcessPool.dll that is to be tested'
    )
    parser.addoption(
        '--process-pool-schema-xml', action='store', default=DEFAULT_POOL_SCHEMA_PATH,
        help='Path to the XML schema for ProcessPool.dll configuration'
    )
    parser.addoption(
        '--dll-bitness', action='store', default=BITNESS_64BIT,
        help='Bitness of DLLs - i.e. x64 or x86'
    )
@pytest.fixture
def asgi_handler_dll(request):
    return request.config.getoption('--asgi-handler-dll')
@pytest.fixture
def process_pool_dll(request):
    return request.config.getoption('--process-pool-dll')
@pytest.fixture
def process_pool_schema_xml(request):
    return request.config.getoption('--process-pool-schema-xml')
@pytest.fixture
def dll_bitness(request):
    if request.config.getoption('--dll-bitness') == BITNESS_64BIT:
        return '64'
    else:
        return '32'


# Taken from pytest docs - makes report available in fixture.
@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # execute all other hooks to obtain the report object
    outcome = yield
    if call.when == 'call':
        rep = outcome.get_result()
        setattr(item, "_report", rep)


# Install/uninstall our section in applicationHost.config.
def _run_js(js):
    handle, path = tempfile.mkstemp(suffix='.js')
    os.close(handle)
    with open(path, 'w') as f:
        f.write("""
            var ahwrite = new ActiveXObject("Microsoft.ApplicationHost.WritableAdminManager");
            var configManager = ahwrite.ConfigManager;
            var appHostConfig = configManager.GetConfigFile("MACHINE/WEBROOT/APPHOST");
            var systemWebServer = appHostConfig.RootSectionGroup.Item("system.webServer");
        """ + js + """
            ahwrite.CommitChanges();
        """)
    subprocess.check_call(['cscript.exe', path])
    os.remove(path)
def add_section(name):
    _run_js("systemWebServer.Sections.AddSection('%s');" % name)
def remove_section(name):
    _run_js("systemWebServer.Sections.DeleteSection('%s');" % name)


# Install/uninstall schema XML files into IIS' installation directory, for the
# modules that have configuration.
# We need to disable file system redirection, as we're running in 32 bit Python.
class DisableFileSystemRedirection(object):
    _disable = ctypes.windll.kernel32.Wow64DisableWow64FsRedirection
    _revert = ctypes.windll.kernel32.Wow64RevertWow64FsRedirection
    def __enter__(self):
        self.old_value = ctypes.c_long()
        self.success = self._disable(ctypes.byref(self.old_value))
    def __exit__(self, type, value, traceback):
        if self.success:
            self._revert(self.old_value)
def install_schema(path, install_path):
    with DisableFileSystemRedirection():
        shutil.copy(path, install_path)
def uninstall_schema(install_path):
    with DisableFileSystemRedirection():
        os.remove(install_path)


def appcmd(*args):
    path = os.path.join('C:\\', 'Windows', 'System32', 'inetsrv', 'appcmd.exe')
    cmd = [path] + list(args)
    logger.debug('Calling: %s', ' '.join(cmd))
    output = subprocess.check_output(cmd)
    logger.debug('Returned: %s', output)
    return output

def uninstall_module(name):
    appcmd('uninstall', 'module', name)

def install_module(name, path, bitness):
    appcmd(
        'install', 'module',
        '/name:%s' % name,
        '/image:%s' % path,
        '/preCondition:bitness%s' % bitness
    )


@pytest.yield_fixture
def asgi_iis_module(asgi_handler_dll, dll_bitness, etw_consumer):
    try:
        install_module(ASGI_MODULE_NAME, asgi_handler_dll, dll_bitness)
        yield
    finally:
        # Allow errors to propogate, as they could affect the ability of other tests
        # to re-install the module.
        uninstall_module(ASGI_MODULE_NAME)


@pytest.yield_fixture
def pool_iis_module(process_pool_dll, dll_bitness, process_pool_schema_xml):
    try:
        install_module(POOL_MODULE_NAME, process_pool_dll, dll_bitness)
        install_schema(process_pool_schema_xml, POOL_SCHEMA_INSTALL_PATH)
        add_section('processPools')
        yield
    finally:
        # Allow errors to propogate, as they could affect the ability of other tests
        # to re-install the module.
        uninstall_module(POOL_MODULE_NAME)
        uninstall_schema(POOL_SCHEMA_INSTALL_PATH)
        remove_section('processPools')


_Site = collections.namedtuple('_Site', ('url', 'https_url', 'ws_url', 'static_path'))
@pytest.yield_fixture
def site(tmpdir, asgi_iis_module, pool_iis_module, dll_bitness):
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
                    <processPools>
                        <process
                            executable="C:\Python27\pythonw.exe"
                            arguments="-c &quot;while True: pass&quot;"
                        />
                    </processPools>
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
        appcmd('add', 'apppool',
            '/name:' + pool_name,
            '/enable32BitAppOnWin64:%s' % ('true' if dll_bitness == '32' else 'false'),
        )
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
