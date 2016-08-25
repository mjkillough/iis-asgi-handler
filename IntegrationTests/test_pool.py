#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function

import pytest

import psutil


def get_processes_for_user(user):
    procs = []
    for proc in psutil.process_iter():
        try:
            # We ignore w3wp.exe (IIS app pool) as it is uninteresting.
            if proc.username() == user and proc.name() != 'w3wp.exe':
                procs.append(proc)
        except psutil.AccessDenied:
            pass
    return procs


def test_pool_launches_process(site, session):
    user = 'IIS APPPOOL\\asgi-test-pool'
    # Initially, there shouldn't be any processes running except the
    # IIS application pool.
    assert get_processes_for_user(user) == []

    # Then when we make the first request, we should see the process
    # get started.
    session.get(site.url + site.static_path).result(timeout=2)

    processes = {
        (proc.name(), tuple(proc.cmdline()))
        for proc in get_processes_for_user(user)
    }
    assert processes == {
        ('pythonw.exe', ('C:\\Python27\\pythonw.exe', '-c', 'while True: pass')),
    }
