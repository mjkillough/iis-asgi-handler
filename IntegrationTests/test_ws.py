#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function

import os
import sys
import time
import urllib

import pytest

from websocket import create_connection


def test_asgi_ws_order_0(site, asgi):
    # websocket.connect['order'] should always be 0
    ws1 = create_connection(site.ws_url, timeout=2)
    asgi_connect1 = asgi.receive_ws_connect()
    assert ws1['order'] == 0
    ws2 = create_connection(site.ws_url, timeout=2)
    asgi_connect2 = asgi.receive_ws_connect()
    assert ws2['order'] == 0


def test_asgi_ws_connect_scheme_ws(site, asgi):
    ws = create_connection(site.ws_url, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    assert asgi_connect['scheme'] == 'ws'


@pytest.mark.skip(reason='Need to provide TLS certificate when setting up IIS site.')
def test_asgi_ws_connect_scheme_wss(site, asgi, session):
    ws = create_connection(site.ws_url, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    assert asgi_connect['scheme'] == 'wss'


@pytest.mark.parametrize('path', [
    '',
    '/onedir/',
    '/missing-trailing',
    '/multi/level/with/file.txt'
])
def test_asgi_ws_connect_path(path, site, asgi):
    ws = create_connection(site.ws_url + path, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    # IIS normalizes an empty path to '/'
    path = path if path else '/'
    assert asgi_connect['path'] == path


@pytest.mark.parametrize('qs_parts', [
    None, # means 'do not append ? to url'
    [],
    [('a', 'b')],
    [('a', 'b'), ('c', 'd')],
    [('a', 'b c')],
])
def test_asgi_ws_connect_querystring(qs_parts, site, asgi, session):
    qs = ''
    if qs_parts is not None:
        qs = '?' + urllib.urlencode(qs_parts)
    ws = create_connection(site.ws_url + qs, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    # IIS gives a query string of '' when provided with '?'
    expected_qs = '' if qs == '?' else qs
    assert asgi_connect['query_string'] == expected_qs.encode('utf-8')


@pytest.mark.parametrize('headers', [
    {'User-Agent': 'Custom User-Agent'}, # 'Known' header
    {'X-Custom-Header': 'Value'},
])
def test_asgi_ws_connect_headers(headers, site, asgi, session):
    ws = create_connection(site.ws_url, header=headers, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    for name, value in headers.items():
        encoded_header = [name.encode('utf-8'), value.encode('utf-8')]
        assert encoded_header in asgi_connect['headers']


def test_ws_basic(site, asgi):
    ws = create_connection(site.ws_url, timeout=5)
    channel, data = asgi.channels.receive_many(['websocket.connect'], block=True)
    print(channel, data)
    ws.send("Hello, world");
    channel, data = asgi.channels.receive_many(['websocket.receive'], block=True)
    print(channel, data)
    assert False


# - Receive data of various sizes
# - websocket.receive has same reply_channel as websocket.connect
# - websocket.receive has same path as websocket.connect
# - websocket.receive has bytes/text depending on utf8
# - websocket.receive multiple times with same connection
# - websocket.receive from multiple connections at once


if __name__ == '__main__':
    # Should really sys.exit() this, but it causes Visual Studio
    # to eat the output. :(
    pytest.main(['--ignore', 'env1/', '-x', 'test_ws.py'])
