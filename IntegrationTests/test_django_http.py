#!/usr/bin/env python
# encoding: utf-8

"""Tests to check that we work with Django/Channels.

Tests in test_asgi_*.py are preferable, as they're much easier to debug.
The tests in here are a bit more complicated, but check we actually work
when run under Django channels.
"""

from __future__ import unicode_literals, print_function

import pytest

from django import http
from django.conf.urls import url


def test_django_http_returns_200_with_body(site, django_worker, session):
    body = 'a body'
    def view(request):
        return http.HttpResponse(body)
    django_worker.set_urlconfs([url('^$', view)])
    django_worker.start()

    future = session.get(site.url)
    resp = future.result(timeout=2)
    assert resp.status_code == 200
    assert resp.text == body


def test_django_http_returns_redirect(site, django_worker, session):
    redirect_url = 'a/path/'
    def view(request):
        return http.HttpResponseRedirect(redirect_url)
    django_worker.set_urlconfs([url('^$', view)])
    django_worker.start()

    future = session.get(site.url, allow_redirects=False)
    resp = future.result(timeout=2)
    assert resp.status_code == 302
    assert 'location' in resp.headers
    assert resp.headers['location'] == redirect_url


def test_django_http_request_headers(site, django_worker, session):
    def view(request):
        return http.HttpResponse(request.META.get('HTTP_X_MY_HEADER', ''))
    django_worker.set_urlconfs([url('^$', view)])
    django_worker.start()

    expected_value = 'a string'
    headers = {'X-My-Header': expected_value}
    future = session.get(site.url, headers=headers)
    resp = future.result(timeout=2)
    assert resp.status_code == 200
    assert resp.text == expected_value


def test_django_http_response_headers(site, django_worker, session):
    header_name = 'X-My-Header'
    header_value = 'response string'
    def view(request):
        resp = http.HttpResponse('')
        resp['X-My-Header'] = header_value
        return resp
    django_worker.set_urlconfs([url('^$', view)])
    django_worker.start()

    future = session.get(site.url)
    resp = future.result(timeout=2)
    assert resp.status_code == 200
    assert header_name in resp.headers
    assert resp.headers[header_name] == header_value


def test_django_http_two_views(site, django_worker, session):
    def view1(request):
        return http.HttpResponse('view1')
    def view2(request):
        return http.HttpResponse('view2')
    django_worker.set_urlconfs([
        url('^view1$', view1),
        url('^view2$', view2),
    ])
    django_worker.start()

    future1 = session.get(site.url + '/view1')
    future2 = session.get(site.url + '/view2')
    resp1 = future1.result(timeout=2)
    resp2 = future2.result(timeout=2)
    assert resp1.status_code == 200
    assert resp2.status_code == 200
    assert resp1.text == 'view1'
    assert resp2.text == 'view2'
