#!/usr/bin/env python
# encoding: utf-8

import sys
import time
import urllib.parse

import pytest


@pytest.mark.parametrize('method', ['GET', 'GeT', 'POST', 'PUT'])
def test_asgi_http_request_method(method, site, asgi, session):
    session.request(method, site.url)
    asgi_request = asgi.receive_request()
    assert asgi_request['method'] == method.upper()


def test_asgi_http_scheme_http(site, asgi, session):
    session.get(site.url)
    asgi_request = asgi.receive_request()
    assert asgi_request['scheme'] == 'http'


@pytest.mark.skip(reason='Need to provide HTTPS certificate when setting up IIS site.')
def test_asgi_http_scheme_https(site, asgi, session):
    session.get(site.https_url, verify=False)
    asgi_request = asgi.receive_request()
    assert asgi_request['scheme'] == 'https'


# TODO: Test unicode too? Unclear whether we'd really be testing our IIS module, urlencode, or IIS.
@pytest.mark.parametrize('qs_parts', [
    None, # means 'do not append ? to url'
    [],
    [('a', 'b')],
    [('a', 'b'), ('c', 'd')],
    [('a', 'b c')],
])
def test_asgi_http_querystring(qs_parts, site, asgi, session):
    qs = ''
    if qs_parts is not None:
        qs = '?' + urllib.parse.urlencode(qs_parts)
    session.get(site.url + qs)
    asgi_request = asgi.receive_request()
    # IIS gives a query string of '' when provided with '?'
    expected_qs = '' if qs == '?' else qs
    assert asgi_request['query_string'] == expected_qs.encode('utf-8')


# TODO: Test for unicode too?
@pytest.mark.parametrize('body', [
    '',
    'this is a body',
])
def test_asgi_http_body(body, site, asgi, session):
    session.post(site.url, data=body)
    asgi_request = asgi.receive_request()
    # assert asgi_request['body'] == body


@pytest.mark.parametrize('headers', [
    {},
    {'User-Agent': 'Custom User-Agent', 'X-Custom-Header': 'Value'},
])
def test_asgi_http_headers(headers, site, asgi, session):
    future = session.get(site.url, headers=headers)
    asgi_request = asgi.receive_request()
    assert len(headers) == len(asgi_request['headers'])
    for name, value in headers.items():
        encoded_header = [name.encode('utf-8'), value.encode('utf-8')]
        assert encoded_header in asgi_request['headers']


if __name__ == '__main__':
    # Should really sys.exit() this, but it causes Visual Studio
    # to eat the output. :(
    pytest.main('--ignore env/')
