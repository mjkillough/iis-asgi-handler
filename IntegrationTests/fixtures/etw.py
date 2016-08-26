#!/usr/bin/env python
# encoding: utf-8

from __future__ import unicode_literals, print_function, absolute_import

import datetime
import os
import threading
import time

import pytest

import etw
import etw.descriptors.event
import etw.descriptors.field


def Consumer(guid):
    """Defines boiler-plate classes for the default 'string' events."""

    class EventDefinitions(object):
        GUID = guid
        StringEvent = (GUID, 0)

    class EventProvider(etw.descriptors.event.EventCategory):
        GUID = EventDefinitions.GUID
        VERSION = 0

        class StringEventClass(etw.descriptors.event.EventClass):
            _event_types_ = [EventDefinitions.StringEvent]
            _fields_ = [('data', etw.descriptors.field.WString)]

    class _Consumer(etw.EventConsumer):
        def __init__(self, *args, **kwargs):
            super(_Consumer, self).__init__(*args, **kwargs)
            self.events = []

        @etw.EventHandler(EventDefinitions.StringEvent)
        def on_event_message(self, event):
            self.events.append(event)

        def get_report(self):
            return '\r\n'.join(
                '%s P%04dT%04d %s' % (
                    datetime.datetime.fromtimestamp(event.time_stamp).isoformat(),
                    event.process_id, event.thread_id, event.data
                )
                for event in self.events
            )

    return _Consumer()


class ConsumerThread(threading.Thread):
    def __init__(self, source, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.source = source

    def run(self):
        self.source.Consume()


class EtwSession(object):

    def __init__(self, guid, session_name):
        self.guid = guid
        self.session_name = session_name

    def start(self):
        # We use REAL_TIME and do not specify a LogFile.
        props = etw.TraceProperties()
        props_struct = props.get()
        props_struct.contents.LogFileMode = etw.evntcons.PROCESS_TRACE_MODE_REAL_TIME
        props_struct.contents.LogFileNameOffset = 0
        self.controller = etw.TraceController()
        self.controller.Start(self.session_name, props)
        self.controller.EnableProvider(
            etw.evntrace.GUID(self.guid),
            etw.evntrace.TRACE_LEVEL_VERBOSE
        )
        self.consumer = Consumer(self.guid)
        self.event_source = etw.TraceEventSource([self.consumer])
        self.event_source.OpenRealtimeSession(self.session_name)
        # We need a separate thread to consume the events.
        self.thread = ConsumerThread(self.event_source)
        self.thread.start()

    def stop(self):
        # Calling .Close() will normally return ERROR_CTX_CLOSE_PENDING as we're
        # using REAL_TIME. It may take a little while for the final events to come in.
        try:
            self.event_source.Close()
        except WindowsError as e:
            if e.errno != 7007: # ERROR_CTX_CLOSE_PENDING
                raise
        # Wait for it to finish consuming events.
        self.thread.join()
        self.controller.DisableProvider(etw.evntrace.GUID(self.guid))
        self.controller.Stop()

    def __enter__(self):
        self.start()
        return self
    def __exit__(self, *args):
        self.stop()


def _add_report_for_fail(request, report_title, etw_session):
    # If the test failed, append a report of all the logs we captured.
    # This relies on our hook to add the report to the test object.
    if hasattr(request.node, '_report'):
        if not request.node._report.passed:
            request.node._report.longrepr.addsection(
                'Captured ETW Logs: %s' % report_title,
                etw_session.consumer.get_report()
            )


asgi_handler_etw_guid = '{b057f98c-cb95-413d-afae-8ed010db73c5}'
process_pool_etw_guid = '{8fc896e8-4370-4976-855b-19b70c976414}'

@pytest.yield_fixture
def asgi_etw_consumer(request):
    session_name = 'AsgiHandlerSession-' + os.urandom(4).encode('hex')
    try:
        with EtwSession(asgi_handler_etw_guid, session_name) as etw_session:
            yield
    finally:
        _add_report_for_fail(request, 'AsgiHandler', etw_session)

@pytest.yield_fixture
def pool_etw_consumer(request):
    session_name = 'ProcessPoolSession-' + os.urandom(4).encode('hex')
    try:
        with EtwSession(process_pool_etw_guid, session_name) as etw_session:
            yield
    finally:
        _add_report_for_fail(request, 'ProcessPool', etw_session)
