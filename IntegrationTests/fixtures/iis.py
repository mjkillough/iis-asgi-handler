#!/usr/bin/env python
# encoding: utf-8

from __future__ import absolute_import

import collections
import ctypes
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile

import pytest

import psutil


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
def asgi_iis_module(asgi_handler_dll, dll_bitness, asgi_etw_consumer):
    try:
        install_module(ASGI_MODULE_NAME, asgi_handler_dll, dll_bitness)
        yield
    finally:
        # Allow errors to propogate, as they could affect the ability of other tests
        # to re-install the module.
        uninstall_module(ASGI_MODULE_NAME)


@pytest.yield_fixture
def pool_iis_module(process_pool_dll, dll_bitness, pool_etw_consumer, process_pool_schema_xml):
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


class SECURITY_DESCRIPTOR(ctypes.Structure):
	# From jaraco.windows
	_fields_ = [
		('Revision', ctypes.c_ubyte),
		('Sbz1', ctypes.c_ubyte),
		('Control', ctypes.c_ushort),
		('Owner', ctypes.c_void_p),
		('Group', ctypes.c_void_p),
		('Sacl', ctypes.c_void_p),
		('Dacl', ctypes.c_void_p),
	]


class SECURITY_ATTRIBUTES(ctypes.Structure):
    # From jaraco.windows
	_fields_ = [
		('nLength', ctypes.c_uint32),
		('lpSecurityDescriptor', ctypes.POINTER(SECURITY_DESCRIPTOR)),
		('bInheritHandle', ctypes.c_bool),
	]


class _ProcessPool(object):

    _pythonw = sys.executable.replace('python.exe', 'pythonw.exe')
    _worker_py = os.path.join(os.path.dirname(__file__), 'worker.py')

    def __init__(self, site):
        self.site = site
        self.name = os.urandom(4).encode('hex')
        self.process = self._pythonw
        self.arguments = [self._worker_py, self.name]
        self._object_prefix = u'Global\\ProcessPool_IntegrationTests_Worker_' + self.name

        # Create a SECURITY_DESCRIPTOR that grants everyone access: we'll be sharing a sempahore
        # and event between user sessions.
        sd = SECURITY_DESCRIPTOR()
        ctypes.windll.advapi32.InitializeSecurityDescriptor(ctypes.byref(sd), 1)
        ctypes.windll.advapi32.SetSecurityDescriptorDacl(ctypes.byref(sd), True, None, False)
        sa = SECURITY_ATTRIBUTES()
        sa.nLength = ctypes.sizeof(sa)
        sa.lpSecurityDescriptor = ctypes.pointer(sd)
        sa.bInheritHandle = False

        # Create a shared semaphore which we can use to count how many processes have run and
        # an event to stop processes. The event is declared with auto-reset, so that each time
        # it is singaled, a single process exits.
        self.semaphore = ctypes.windll.kernel32.CreateSemaphoreW(
            ctypes.byref(sa), 0x0, 0xFFFF,
            self._object_prefix + '_Counter'
        )
        assert self.semaphore, ctypes.GetLastError()
        self.exit_event = ctypes.windll.kernel32.CreateEventW(
            ctypes.byref(sa), False, False,
            self._object_prefix + '_Exit'
        )
        assert self.exit_event, ctypes.GetLastError()

    @staticmethod
    def escape_argument(arg):
        # See https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
        if not arg or re.search(r'(["\s])', arg):
            return '"' + arg.replace('"', r'\"') + '"'
        return arg

    @staticmethod
    def get_processes_for_user(user):
        procs = []
        for proc in psutil.process_iter():
            try:
                # We ignore w3wp.exe (IIS app pool) as it is uninteresting.
                if proc.username() == user and proc.name() != 'w3wp.exe':
                    procs.append(proc)
            except psutil.AccessDenied:
                pass
        return {
            (proc.name(), tuple(proc.cmdline()))
            for proc in procs
        }

    @property
    def escaped_arguments(self):
        return ' '.join(map(self.escape_argument, self.arguments))

    @property
    def num_started(self):
        """Returns the number of processes that have been started for this pool.

        Each process increments a semaphore when it starts, so we check the current count.
        """
        # We must increment/decrement the semaphore in order to get the current value.
        current_count = ctypes.c_long()
        ctypes.windll.kernel32.ReleaseSemaphore(self.semaphore, 1, ctypes.byref(current_count))
        ctypes.windll.kernel32.WaitForSingleObject(self.semaphore, 0)
        return current_count.value

    @property
    def num_running(self):
        """Returns the number of processes that are currently running in the pool."""
        # It is unlikely that there are other processes running under the same user with
        # the same command line.
        expected = (os.path.basename(self.process), tuple([self.process] + self.arguments))
        matching = [
            process
            for process in self.get_processes_for_user(self.site.user)
            if process == expected
        ]
        return len(matching)

    def kill_one(self):
        """Signals the event, causing one process to exit"""
        assert ctypes.windll.kernel32.SetEvent(self.exit_event), ctypes.GetLastError()


class _Site(object):

    pool_name = 'asgi-test-pool'
    user = 'IIS APPPOOL\\' + pool_name
    site_name = 'asgi-test-site'
    http_port = 90
    https_port = 91
    http_url = 'http://localhost:%i' % http_port
    https_url = 'http://localhost:%i' % https_port
    ws_url = 'ws://localhost:%i' % http_port
    url = http_url
    static_path = '/static.html'

    def __init__(self, directory, dll_bitness):
        self.directory = directory
        self.dll_bitness = dll_bitness

    def create(self):
        # Create a web.config which sends most requests through
        # our handler. We create a subdirectory that serves static files
        # so that we can test our handler doesn't get in the way
        # of others.
        webconfig = self.directory.join('web.config')
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
        staticfile = self.directory.join('static.html')
        staticfile.write('Hello, world!')
        # Ensure IIS can read the directory. Use icacls to avoid introducing
        # pywin32 dependency.
        for path in [self.directory, webconfig, staticfile]:
            subprocess.check_call(['icacls', str(path), '/grant', 'Users:R'])
        # Add the site with its own app pool. Failing tests can cause the
        # pool to stop, so we create a new one for each test.
        appcmd('add', 'apppool',
            '/name:' + self.pool_name,
            '/enable32BitAppOnWin64:%s' % ('true' if self.dll_bitness == '32' else 'false'),
        )
        appcmd('add', 'site',
            '/name:' + self.site_name,
            '/bindings:http://*:%i,https://*:%i' % (self.http_port, self.https_port),
            '/physicalPath:' + str(self.directory),
        )
        appcmd('set', 'app', self.site_name + '/', '/applicationPool:' + self.pool_name)
        appcmd('unlock', 'config', '-section:system.webServer/handlers')

    def destroy(self):
        appcmd('delete', 'apppool', self.pool_name)
        appcmd('delete', 'site', self.site_name)

    def __enter__(self):
        self.create()
        return self
    def __exit__(self, *args):
        self.destroy()

    def stop(self):
        appcmd('stop', 'site', self.site_name)
    def start(self):
        appcmd('start', 'site', self.site_name)
    def restart(self):
        self.stop()
        self.start()

    def stop_application_pool(self):
        appcmd('stop', 'apppool', self.pool_name)
    def start_application_pool(self):
        appcmd('start', 'apppool', self.pool_name)
    def restart_application_pool(self):
        self.stop_application_pool()
        self.start_application_pool()

    def add_process_pool(self):
        pool = _ProcessPool(self)
        appcmd(
            'set', 'config', self.site_name,
           '/section:system.webServer/processPools',
           '/+[executable=\'%s\',arguments=\'%s\']' % (pool.process, pool.escaped_arguments)
        )
        self.restart()
        return pool

    def clear_process_pools(self):
        appcmd(
            'clear', 'config', self.site_name,
            '/section:system.webServer/processPools',
        )
        self.restart()

@pytest.yield_fixture
def site(tmpdir, asgi_iis_module, pool_iis_module, dll_bitness):
    with _Site(tmpdir, dll_bitness) as site:
        yield site
