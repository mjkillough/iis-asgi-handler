#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function

import pytest


def test_doesnt_interfere_with_other_handlers(site, asgi, session):
    future = session.get(site.url + site.static_path)
    asgi.assert_empty()
    resp = future.result(timeout=2)
    assert resp.text == 'Hello, world!'
    asgi.assert_empty()
