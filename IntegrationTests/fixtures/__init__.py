#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function, absolute_import

import pytest

from .asgi import asgi
from .django_worker import django_worker
from .etw import *
from .iis import *
from .requests import *


# Taken from pytest docs - makes report available in fixture.
@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # execute all other hooks to obtain the report object
    outcome = yield
    if call.when == 'call':
        rep = outcome.get_result()
        setattr(item, "_report", rep)
