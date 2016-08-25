#!/usr/bin/env python
# encoding: utf-8

from __future__ import absolute_import

import concurrent.futures
import logging

import pytest

import requests_futures.sessions

logger = logging.getLogger(__name__)


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
    # IIS on non-server OS only supports 10 concurrent connections, so there is no point
    # starting many more workers.
    executor = _ThreadPoolExecutor(max_workers=10)
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
