#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function

import os
import sys
import time
import urllib

import pytest

from websocket import create_connection


def test_ws_basic(site, asgi):
    ws = create_connection(site.ws_url, timeout=5)

if __name__ == '__main__':
    # Should really sys.exit() this, but it causes Visual Studio
    # to eat the output. :(
    pytest.main(['--ignore', 'env1/', '-x', 'test_ws.py'])
