#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function, absolute_import

import os
import logging
import subprocess
import collections

import pytest
import asgi_redis
import requests_futures.sessions

from .etw import *


logger = logging.getLogger(__name__)

MODULE_NAME = 'RedisAsgiHandler'
DLL_PATH = os.path.join('C:\\', 'dev', 'RedisAsgiHandler', 'x64', 'Debug', 'RedisAsgiHandler.dll')


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


@pytest.fixture
def session():
    return requests_futures.sessions.FuturesSession()
