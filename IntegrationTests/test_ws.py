#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function

import os
import sys
import time
import urllib
import collections

import pytest

from websocket import create_connection


def test_asgi_ws_order_0(site, asgi):
    # websocket.connect['order'] should always be 0
    ws1 = create_connection(site.ws_url, timeout=2)
    asgi_connect1 = asgi.receive_ws_connect()
    assert asgi_connect1['order'] == 0
    ws2 = create_connection(site.ws_url, timeout=2)
    asgi_connect2 = asgi.receive_ws_connect()
    assert asgi_connect2['order'] == 0


def test_asgi_ws_connect_scheme_ws(site, asgi):
    ws = create_connection(site.ws_url, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    assert asgi_connect['scheme'] == 'ws'


@pytest.mark.skip(reason='Need to provide TLS certificate when setting up IIS site.')
def test_asgi_ws_connect_scheme_wss(site, asgi):
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
def test_asgi_ws_connect_querystring(qs_parts, site, asgi):
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
def test_asgi_ws_connect_headers(headers, site, asgi):
    ws = create_connection(site.ws_url, header=headers, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    for name, value in headers.items():
        encoded_header = [name.encode('utf-8'), value.encode('utf-8')]
        assert encoded_header in asgi_connect['headers']


def test_asgi_ws_receive(site, asgi):
    ws = create_connection(site.ws_url, timeout=2)
    ws.send(b'Hello, world!')
    asgi_receive = asgi.receive_ws_data()
    assert asgi_receive['bytes'] == b'Hello, world!'


def test_asgi_ws_receive_has_correct_channel(site, asgi):
    ws = create_connection(site.ws_url, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    ws.send('some data')
    asgi_receive = asgi.receive_ws_data()
    assert asgi_connect['reply_channel'].startswith('websocket.send')
    assert asgi_connect['reply_channel'] == asgi_receive['reply_channel']


def test_asgi_ws_receive_has_correct_path(site, asgi):
    path = '/a/path/'
    ws = create_connection(site.ws_url + path, timeout=2)
    asgi_connect = asgi.receive_ws_connect()
    ws.send('some data')
    asgi_receive = asgi.receive_ws_data()
    assert asgi_connect['path'] == path
    assert asgi_connect['path'] == asgi_receive['path']


def test_asgi_ws_receive_multiple_times_same_connection(site, asgi):
    ws = create_connection(site.ws_url, timeout=2)
    # Check we can receive multiple frames from the websocket connection,
    # and ensure we can still send more data after we've received some.
    for _ in range(2):
        for i in range(10):
            ws.send(b'%i' % i)
        for i in range(10):
            asgi_receive = asgi.receive_ws_data()
            assert asgi_receive['bytes'] == b'%i' % i


def test_asgi_ws_concurrent_connection(site, asgi):
    # TODO: Paramaterize this test and have it create more connections when
    # run on Windows Server, where we can have >10 concurrent connections.
    wses = [create_connection(site.ws_url, timeout=2) for _ in range(10)]
    for i, ws in enumerate(wses):
        for j in range(10):
            ws.send(b'%i-%i' % (j, i))

    asgi_msgs = collections.defaultdict(set)
    for i in range(10 * len(wses)):
        asgi_msg = asgi.receive_ws_data()
        connection, msg_num = asgi_msg['bytes'].split(b'-')
        asgi_msgs[connection].add(msg_num)

    assert len(asgi_msgs) == 10
    for asgi_msg_set in asgi_msgs.itervalues():
        assert asgi_msg_set == set(b'%i' % i for i in range(10))

# - Receive data of various sizes
# - websocket.receive has bytes/text depending on utf8
# - websocket.receive multiple times with same connection
# - websocket.receive from multiple connections at once


if __name__ == '__main__':
    # Should really sys.exit() this, but it causes Visual Studio
    # to eat the output. :(
    pytest.main(['--ignore', 'env1/', '-x', 'test_ws.py'])
