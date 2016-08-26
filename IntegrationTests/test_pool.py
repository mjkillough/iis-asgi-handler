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
    return {
        (proc.name(), tuple(proc.cmdline()))
        for proc in procs
    }


_pythonw = 'C:\\Python27\\pythonw.exe'


def test_pool_launches_process(site, session):
    site.add_process_pool(_pythonw, '-c "while True: pass"')
    before = get_processes_for_user(site.user)

    # Make a request so the pool gets started.
    session.get(site.url + site.static_path).result(timeout=2)

    after = get_processes_for_user(site.user)
    assert after == before | {
        ('pythonw.exe', (_pythonw, '-c', 'while True: pass')),
    }


@pytest.mark.xfail
def test_pool_terminated_when_site_stopped(site, session):
    site.add_process_pool(_pythonw, '-c "while True: pass"')
    before = get_processes_for_user(site.user)

    # Make a request so the pool gets started.
    session.get(site.url + site.static_path).result(timeout=2)

    after1 = get_processes_for_user(site.user)
    assert after1 == before | {
        ('pythonw.exe', (_pythonw, '-c', 'while True: pass')),
    }

    site.stop()
    after2 = get_processes_for_user(site.user)
    assert before == after2



def test_pool_two_pools(site, session):
    site.add_process_pool(_pythonw, '-c "while True: a=1"')
    site.add_process_pool(_pythonw, '-c "while True: a=2"')
    before = get_processes_for_user(site.user)

    # Make a request so the pool gets started.
    session.get(site.url + site.static_path).result(timeout=2)

    after = get_processes_for_user(site.user)
    assert after == before | {
        ('pythonw.exe', (_pythonw, '-c', 'while True: a=1')),
        ('pythonw.exe', (_pythonw, '-c', 'while True: a=2')),
    }


def test_pool_add_to_existing_site(site, session):
    site.add_process_pool(_pythonw, '-c "while True: a=1"')
    before = get_processes_for_user(site.user)

    # Make a request so the pool gets started.
    session.get(site.url + site.static_path).result(timeout=2)

    after1 = get_processes_for_user(site.user)
    assert after1 == before | {
        ('pythonw.exe', (_pythonw, '-c', 'while True: a=1')),
    }

    site.add_process_pool(_pythonw, '-c "while True: a=2"')

    # Make another request so the pool gets started again.
    session.get(site.url + site.static_path).result(timeout=2)

    after2 = get_processes_for_user(site.user)
    assert after2 == before | {
        ('pythonw.exe', (_pythonw, '-c', 'while True: a=1')),
        ('pythonw.exe', (_pythonw, '-c', 'while True: a=2')),
    }
