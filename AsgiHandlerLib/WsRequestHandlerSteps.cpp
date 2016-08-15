// This file contains the steps for WsRequestHandler. These deal with setting up
// the WebSocket by accepting the UPGRADE request and sending the websocket.connect
// ASGI message. They do not deal with the general reading/writing from websockets.

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <ppltasks.h>

#include "WsRequestHandlerSteps.h"
#include "RequestHandler.h"
#include "ChannelLayer.h"


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
    return std::make_unique<SendConnectToApplicationStep>(m_handler, std::move(m_asgi_connect_msg));
}


// Connection Pipeline - SendConnectToApplicationStep

StepResult SendConnectToApplicationStep::Enter()
{
    auto task = concurrency::create_task([this]() {
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, *m_asgi_connect_msg);
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
