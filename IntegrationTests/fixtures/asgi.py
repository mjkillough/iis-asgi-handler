#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function, absolute_import

import pytest
import asgi_redis


class RedisChannelLayer(object):

    def __init__(self):
        # TODO: Use a custom prefix and configure IIS module to use it too.
        self.channels = asgi_redis.RedisChannelLayer()
        self.channels.flush()

    def _receive_one(self, channel_name):
        channel, data = self.channels.receive_many([channel_name], block=True)
        assert channel == channel_name
        return data

    def receive_request(self):
        return self._receive_one('http.request')

    def receive_ws_connect(self):
        return self._receive_one('websocket.connect')

    def receive_ws_data(self):
        return self._receive_one('websocket.receive')

    def send(self, channel, msg):
        self.channels.send(channel, msg)

    def assert_empty(self):
        channel, asgi_request = self.channels.receive_many(['http.request'], block=False)
        assert channel == None
        assert asgi_request == None


@pytest.fixture
def asgi():
    return RedisChannelLayer()
