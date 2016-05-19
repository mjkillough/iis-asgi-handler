#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <iiswebsocket.h>
#include <ppltasks.h>

#include "WsRequestHandler.h"
#include "AsgiHttpRequestMsg.h"

#include "Logger.h"


REQUEST_NOTIFICATION_STATUS WsRequestHandler::OnExecuteRequestHandler()
{
    m_current_connect_step = std::make_unique<AcceptWebSocketStep>(*this);

    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->Enter() being called";
    auto result = m_current_connect_step->Enter();
    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->Enter() = " << result;

    return HandlerStateMachine(m_current_connect_step, result);
}

REQUEST_NOTIFICATION_STATUS WsRequestHandler::OnAsyncCompletion(IHttpCompletionInfo* completion_info)
{
    HRESULT hr = completion_info->GetCompletionStatus();
    DWORD bytes = completion_info->GetCompletionBytes();

    StartReadWritePipelines();

    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() being called";
    auto result = m_current_connect_step->OnAsyncCompletion(hr, bytes);
    logger.debug() << typeid(*m_current_connect_step.get()).name() << "->OnAsyncCompletion() = " << result;

    return HandlerStateMachine(m_current_connect_step, result);
}

void WsRequestHandler::StartReadWritePipelines()
{
    m_read_pump.Start();
}


// Connection Pipeline - AcceptWebSocketStep

StepResult AcceptWebSocketStep::Enter()
{
    // Setting the status as 101.
    auto resp = m_http_context->GetResponse();
    resp->SetStatus(101, "");

    // ... and Flushing causes the IIS WebSocket module to kick into action.
    // We have to set fMoreData=true in order for it to work.
    DWORD num_bytes = 0;
    BOOL completion_expected = false;
    HRESULT hr = resp->Flush(true, true, &num_bytes, &completion_expected);
    if (FAILED(hr)) {
        logger.debug() << "Flush() = " << hr;
        return kStepFinishRequest;
    }

    if (!completion_expected) {
        return OnAsyncCompletion(S_OK, num_bytes);
    }

    return kStepAsyncPending;
}

StepResult AcceptWebSocketStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    // Nothing to do but to go to the next step. (We don't know how many bytes
    // were in the response, so num_bytes is meaningless to us).
    // Our caller will handle FAILED(hr).
    return kStepGotoNext;
}

std::unique_ptr<RequestHandlerStep> AcceptWebSocketStep::GetNextStep()
{
    return std::make_unique<SendConnectToApplicationStep>(m_handler);
}


// Connection Pipeline - SendConnectToApplicationStep

StepResult SendConnectToApplicationStep::Enter()
{
    auto task = concurrency::create_task([this]() {
        AsgiHttpRequestMsg msg;
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, msg);
        m_channels.Send("websocket.connect", buffer);
    }).then([this]() {
        logger.debug() << "SendConnectToApplicationStep calling PostCompletion()";
        m_http_context->PostCompletion(0);
    });

    return kStepAsyncPending;
}

StepResult SendConnectToApplicationStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    if (FAILED(hr)) {
        // TODO: Call an Error() or something.
        return kStepFinishRequest;
    }

    // We return kStepAsyncPending so that the request is kept open. We'll
    // get callbacks from IWebSocketContext from here onwards (not directly from IIS'
    // main request pipeline).
    return kStepAsyncPending;
}

std::unique_ptr<RequestHandlerStep> SendConnectToApplicationStep::GetNextStep()
{
    // TODO: Go to DummyStep in order to make sure we release any resources held
    //       by this step?
    throw std::runtime_error("SendConnectToApplicationStep::GetNextStep() should never get called.");
}


WsIoPump::WsIoPump(WsRequestHandler& handler)
    : logger(handler.logger), m_channels(handler.m_channels),
      m_http_context(handler.m_http_context)
{ }


void WsReadPump::Start()
{
    auto http_context3 = static_cast<IHttpContext3*>(m_http_context);
    m_ws_context = static_cast<IWebSocketContext*>(
        http_context3->GetNamedContextContainer()->GetNamedContext(L"websockets")
    );

    ReadAsync();
}

void WsReadPump::ReadAsync()
{
    logger.debug() << "ReadAsync()";

    BOOL completion_expected = false;
    while (!completion_expected) {
        DWORD num_bytes = m_msg.data.size();
        BOOL utf8 = false, final_fragment = false, close = false;

        HRESULT hr = m_ws_context->ReadFragment(
            m_msg.data.data(), &num_bytes, true, &utf8, &final_fragment, &close,
            ReadCallback, this, &completion_expected
        );
        if (FAILED(hr)) {
            logger.debug() << "ReadFragment() = " << hr;
            // TODO: Call an Error() or something.
            // TODO: Figure out how to close the request from here.
        }

        if (!completion_expected) {
            logger.debug() << "ReadFragment() returned completion_expected=false";
            ReadAsyncComplete(S_OK, num_bytes, utf8, final_fragment, close);
        }
    }

    logger.debug() << "ReadAsync() returning";
}

void WsReadPump::ReadAsyncComplete(HRESULT hr, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close)
{
    logger.debug() << "ReadAsyncComplete() " << num_bytes << "  " << utf8 << " " << final_fragment << " " << close;

    if (FAILED(hr)) {
        logger.debug() << "ReadAsyncComplete() hr = " << hr;
        // TODO: Figure out how to propogate an error from here.
    }
    // TODO: Handle close.

    m_msg.data_size += num_bytes;
    m_msg.utf8 = utf8;

    if (final_fragment) {
        // Don't bother re-sizing the buffer to the correct size before sending.
        // The msgpack serializer will take care of it.
        SendToApplicationAsync();
    } else {
        // If we're almost at the end of the buffer, increase the buffer size.
        if (m_msg.data_size >= (m_msg.data.size() - AsgiWsReceiveMsg::BUFFER_CHUNK_INCREASE_THRESHOLD)) {
            m_msg.data.resize(m_msg.data.size() + AsgiWsReceiveMsg::BUFFER_CHUNK_SIZE);
        }
    }
}

void WsReadPump::SendToApplicationAsync()
{
    // Send synchronously for now.
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, m_msg);
    m_channels.Send("websocket.receive", buffer);

    // Reset for the next msg.
    m_msg.data.resize(AsgiWsReceiveMsg::BUFFER_CHUNK_SIZE);
    m_msg.order += 1;
}

void WINAPI WsReadPump::ReadCallback(HRESULT hr, VOID* context, DWORD num_bytes, BOOL utf8, BOOL final_fragment, BOOL close)
{
    auto read_pump = static_cast<WsReadPump*>(context);
    read_pump->ReadAsyncComplete(hr, num_bytes, utf8, final_fragment, close);
    read_pump->ReadAsync();
}
