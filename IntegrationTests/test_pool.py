#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function

import os
import datetime
import sys

import pytest


def wait_until(condition, timeout=2):
    """Waits until the specified condition function returns true, or the timeout
    (in seconds) elapses."""
    end = datetime.datetime.utcnow() + datetime.timedelta(seconds=timeout)
    while datetime.datetime.utcnow() < end:
        if condition():
            return True
    return False


def start_pools(session, site):
    """Make a request to cause the Process Pools to get created."""
    session.get(site.url + site.static_path).result(timeout=2)


def test_pool_launches_process(site, session):
    pool = site.add_process_pool()
    assert pool.num_started == 0
    start_pools(session, site)
    assert wait_until(lambda: pool.num_started == 1)
    assert pool.num_running == 1


def test_pool_terminated_when_site_stopped(site, session):
    pool = site.add_process_pool()
    start_pools(session, site)
    assert wait_until(lambda: pool.num_started == 1)
    assert pool.num_running == 1
    site.stop_application_pool()
    assert wait_until(lambda: pool.num_running == 0)


def test_pool_two_pools(site, session):
    pool1 = site.add_process_pool()
    pool2 = site.add_process_pool()
    start_pools(session, site)
    assert wait_until(lambda: pool1.num_started == 1)
    assert pool1.num_running == 1
    assert wait_until(lambda: pool2.num_started == 1)
    assert pool2.num_running == 1


def test_pool_add_to_existing_site(site, session):
    pool1 = site.add_process_pool()
    start_pools(session, site)
    assert wait_until(lambda: pool1.num_started == 1)
    assert pool1.num_running == 1

    pool2 = site.add_process_pool()
    start_pools(session, site)
    # This will cause pool1 to restart, so the num_started will be 2.
    assert wait_until(lambda: pool1.num_started == 2)
    assert pool1.num_running == 1
    assert wait_until(lambda: pool2.num_started == 1)
    assert pool2.num_running == 1


def test_pool_exiting_process_restarted(site, session):
    pool = site.add_process_pool()
    start_pools(session, site)
    assert wait_until(lambda: pool.num_started == 1)
    assert pool.num_running == 1
    pool.kill_one()
    assert wait_until(lambda: pool.num_started == 2)
    assert pool.num_running == 1
