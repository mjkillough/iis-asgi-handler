#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function, absolute_import

import os
import threading
import datetime

import pytest

import etw
import etw.descriptors.event
import etw.descriptors.field


class EventDefinitions(object):
    GUID = '{b057f98c-cb95-413d-afae-8ed010db73c5}'
    StringEvent = (GUID, 0)


class EventProvider(etw.descriptors.event.EventCategory):
    GUID = EventDefinitions.GUID
    VERSION = 0

    class StringEventClass(etw.descriptors.event.EventClass):
        _event_types_ = [EventDefinitions.StringEvent]
        _fields_ = [('data', etw.descriptors.field.WString)]


class Consumer(etw.EventConsumer):
    def __init__(self, *args, **kwargs):
        super(etw.EventConsumer, self).__init__(*args, **kwargs)
        self.events = []

    @etw.EventHandler(EventDefinitions.StringEvent)
    def on_event_message(self, event):
        self.events.append(event)

    def get_report(self):
        return '\r\n'.join(
            '%s: %s' % (
                datetime.datetime.fromtimestamp(event.time_stamp).isoformat(),
                event.data
            )
            for event in self.events
        )


class ConsumerThread(threading.Thread):
    def __init__(self, source, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.source = source

    def run(self):
        self.source.Consume()


# Taken from pytest docs - makes report available in fixture.
@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # execute all other hooks to obtain the report object
    outcome = yield
    if call.when == 'call':
        rep = outcome.get_result()
        setattr(item, "_etw_consumer_report", rep)


@pytest.yield_fixture
def etw_consumer(request):
    session_name = 'RedisAsgiHandlerSession-' + os.urandom(4).encode('hex')
    try:
        # We use REAL_TIME and do not specify a LogFile.
        props = etw.TraceProperties()
        props_struct = props.get()
        props_struct.contents.LogFileMode = etw.evntcons.PROCESS_TRACE_MODE_REAL_TIME
        props_struct.contents.LogFileNameOffset = 0
        controller = etw.TraceController()
        controller.Start(session_name, props)
        controller.EnableProvider(
            etw.evntrace.GUID(EventDefinitions.GUID),
            etw.evntrace.TRACE_LEVEL_VERBOSE
        )
        consumer = Consumer()
        try:
            source = etw.TraceEventSource([consumer])
            source.OpenRealtimeSession(session_name)
            # We need a separate thread to consume the events.
            thread = ConsumerThread(source)
            thread.start()
            yield
        finally:
            # Calling .Close() will normally return ERROR_CTX_CLOSE_PENDING as we're
            # using REAL_TIME. It may take a little while for the final events to come in.
            try:
                source.Close()
            except WindowsError as e:
                if e.errno != 7007: # ERROR_CTX_CLOSE_PENDING
                    raise
            # Wait for it to finish consuming events.
            thread.join()
            # If the test failed, append a report of all the logs we captured.
            # This relies on our hook to add the report ot the test object.
            if hasattr(request.node, '_etw_consumer_report'):
                if not request.node._etw_consumer_report.passed:
                    request.node._etw_consumer_report.longrepr.addsection(
                        'Captured ETW Logs',
                        consumer.get_report()
                    )
    finally:
        controller.DisableProvider(etw.evntrace.GUID(EventDefinitions.GUID))
        controller.Stop()
