#!/usr/bin/env python
# encoding: utf-8

import threading

import pytest

from django.conf import settings
import django.urls

import asgi_redis
import channels.worker
import channels.asgi


class WorkerWrapper(object):
    """Wraper channels.worker.Worker and provides a nice interface for tests."""

    def __init__(self, worker):
        self.worker = worker

    def set_urlconfs(self, urlconfs):
        class UrlConf(object):
            urlpatterns = urlconfs
        settings.ROOT_URLCONF = UrlConf()
        django.urls.clear_url_caches()

    def start(self):
        self.thread = threading.Thread(target=self.worker.run)
        self.thread.start()

    def stop(self):
        self.worker.termed =True
        self.thread.join()


@pytest.yield_fixture
def django_worker(request):
    if not settings.configured:
        settings.configure(DEBUG=True)

    channel_layer = asgi_redis.RedisChannelLayer()
    channel_layer_wrapper = channels.asgi.ChannelLayerWrapper(
        channel_layer, 'default', dict()
    )
    channel_layer_wrapper.router.check_default()
    worker = channels.worker.Worker(channel_layer_wrapper, signal_handlers=False)

    worker_wrapper = WorkerWrapper(worker)
    try:
        yield worker_wrapper
    finally:
        worker_wrapper.stop()
