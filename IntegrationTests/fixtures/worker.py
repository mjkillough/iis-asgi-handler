#!/usr/bin/env python
# encoding: utf-8

"""Simple worker process for use in test_pool.py"""

import atexit
import ctypes
import sys


WAIT_OBJECT_0 = 0x0
WAIT_TIMEOUT = 0x102
INFINITE = 0xFFFFFFFF
SYNCHRONIZE = 0x00100000
EVENT_MODIFY_STATE = 0x2


if __name__ == '__main__':
    pool_name = sys.argv[1]
    object_prefix = u'Global\\ProcessPool_IntegrationTests_Worker_' + pool_name

    # The tests will signal this event when they want a process to exit.
    # The event is set to auto-reset, so only one process will exit each time it is signaled.
    exit_event = ctypes.windll.kernel32.OpenEventW(
        SYNCHRONIZE, False,
        object_prefix + '_Exit'
    )
    assert exit_event, ctypes.GetLastError()

    # Increment a semaphore so the tests can see how many processes have started running.
    # As we want to use it as a counter, we start at 0 and each process ReleaseSemaphores()
    # to register itself.
    semaphore = ctypes.windll.kernel32.OpenSemaphoreW(
        SYNCHRONIZE | EVENT_MODIFY_STATE, False,
        object_prefix + '_Counter'
    )
    assert semaphore, ctypes.GetLastError()
    ctypes.windll.kernel32.ReleaseSemaphore(semaphore, 1, None)

    # Continually wait on the exit_event until it is signaled.
    while True:
        wait_result = ctypes.windll.kernel32.WaitForSingleObject(exit_event, INFINITE)
        if wait_result == WAIT_OBJECT_0:
            break

