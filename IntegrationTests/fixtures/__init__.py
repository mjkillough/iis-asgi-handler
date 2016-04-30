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

from .etw import *


logger = logging.getLogger(__name__)

MODULE_NAME = 'RedisAsgiHandler'
DLL_PATH = os.path.join('C:\\', 'dev', 'RedisAsgiHandler', 'x64', 'Debug', 'RedisAsgiHandler.dll')


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
    print(path)
    cmd = [path] + list(args)
    return subprocess.check_output(cmd)

def uninstall_module():
    appcmd('uninstall', 'module', MODULE_NAME)

def install_module():
    appcmd('install', 'module', '/name:%s' % MODULE_NAME, '/image:%s' % DLL_PATH)


@pytest.yield_fixture
def install_iis_module(etw_consumer):
    try:
        install_module()
        yield
    finally:
        # Allow errors to propogate, as they could affect the ability of other tests
        # to re-install the module.
        print('uninstalling')
        uninstall_module()

_Site = collections.namedtuple('_Site', ('url', 'https_url'))
@pytest.yield_fixture
def site(tmpdir, install_iis_module):
    # TODO: - Randomize site name/port
    #       - Remove any existing site before trying to create one
    #       - Use a custom app pool and ensure it is started
    try:
        # Ensure IIS can read the directory. Use icacls to avoid introducing
        # pywin32 dependency.
        subprocess.check_call(['icacls', str(tmpdir), '/grant', 'Users:R'])
        # Add the site and wait for it to start.
        appcmd('add', 'site',
            '/name:int-test-site', '/bindings:http://*:99,https://*:100',
            '/physicalPath:' + str(tmpdir)
        )
        while True: # TODO: Timeout & delay between loops.
            output = appcmd('list', 'site', 'int-test-site')
            if b'Started' in output:
                break

        yield _Site('http://localhost:99', 'https://localhost:100')
    finally:
        appcmd('delete', 'site', 'int-test-site')


@pytest.fixture
def asgi():
    # TODO: Use a custom prefix and configure IIS module to use it too.
    class _ChannelsWrapper(object):
        def __init__(self, asgi):
            self.asgi = asgi
        def receive_request(self):
            channel, asgi_request = self.asgi.receive_many(['http.request'], block=True)
            assert channel == 'http.request'
            return asgi_request

    return _ChannelsWrapper(asgi_redis.RedisChannelLayer())


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
    executor = _ThreadPoolExecutor(max_workers=2)
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
