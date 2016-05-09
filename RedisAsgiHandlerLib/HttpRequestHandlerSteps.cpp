#pragma once

#include <stdexcept>
#include <typeinfo>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "msgpack.hpp"

#include "HttpRequestHandlerSteps.h"
#include "HttpRequestHandler.h"


HttpRequestHandlerStep::HttpRequestHandlerStep(HttpRequestHandler & handler)
    : m_handler(handler), m_http_context(handler.m_http_context),
      logger(handler.logger), m_response_pump(handler.m_response_pump),
      m_channels(handler.m_channels)
{ }


StepResult ReadBodyStep::Enter()
{
    IHttpRequest* request = m_http_context->GetRequest();

    DWORD remaining_bytes = request->GetRemainingEntityBytes();
    if (remaining_bytes == 0) {
        return kStepGotoNext;
    }

    BOOL completion_expected = FALSE;
    while (!completion_expected) {
        DWORD bytes_read;
        HRESULT hr = request->ReadEntityBody(
            m_asgi_request_msg->body.data() + m_body_bytes_read, remaining_bytes,
            true, &bytes_read, &completion_expected
        );
        if (FAILED(hr)) {
            logger.debug() << "ReadEntityBody() = " << hr;
            // TODO: Call an Error() or something.
            return kStepFinishRequest;
        }

        if (!completion_expected) {
            // Operation completed synchronously.
            auto result = OnAsyncCompletion(S_OK, bytes_read);
            // If we need to read more, we might as well do that here, rather than
            // yielding back to the request loop.
            if (result != kStepRerun) {
                return result;
            }
        }

        remaining_bytes = request->GetRemainingEntityBytes();
    }

    return kStepAsyncPending;
}

StepResult ReadBodyStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    if (FAILED(hr)) {
        // TODO: Call an Error() or something.
        return kStepFinishRequest;
    }

    IHttpRequest* request = m_http_context->GetRequest();
    m_body_bytes_read += num_bytes;
    if (request->GetRemainingEntityBytes()) {
        // There's more to do - ask to be re-run.
        return kStepRerun;
    }

    return kStepGotoNext;
}

std::unique_ptr<HttpRequestHandlerStep> ReadBodyStep::GetNextStep()
{
    return std::make_unique<SendToApplicationStep>(
        m_handler, std::move(m_asgi_request_msg)
    );
}


StepResult SendToApplicationStep::Enter()
{
    // TODO: Split into chunked messages.
    auto task = m_channels.Send("http.request", *m_asgi_request_msg);
    task.then([this]() {
        logger.debug() << "SendToApplicationStep calling PostCompletion()";

        // The tests rely on this being the last thing that the callback does.
        m_http_context->PostCompletion(0);
    });

    return kStepAsyncPending;
}

StepResult SendToApplicationStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    // TODO: Consider whether the channel layer can ever give us an error, and whether
    // we should handle it here. (ChannelFull?)
    return kStepGotoNext;
}

std::unique_ptr<HttpRequestHandlerStep> SendToApplicationStep::GetNextStep()
{
    return std::make_unique<WaitForResponseStep>(
        m_handler, m_asgi_request_msg->reply_channel
    );
}


StepResult WaitForResponseStep::Enter()
{
    m_response_pump.AddChannel(m_reply_channel, [this](std::string data) {
        // This is all sorts of wrong.
        m_asgi_response_msg = std::make_unique<AsgiHttpResponseMsg>(
            msgpack::unpack(data.data(), data.length()).get().as<AsgiHttpResponseMsg>()
        );
        m_http_context->PostCompletion(0);

        logger.debug() << "MessagePump gave us a message; PostCompletion() called";
    });

    return kStepAsyncPending;
}

StepResult WaitForResponseStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    IHttpResponse *response = m_http_context->GetResponse();
    response->Clear();
    response->SetStatus(m_asgi_response_msg->status, ""); // Can we pass nullptr for the reason?
    for (auto header : m_asgi_response_msg->headers) {
        std::string header_name = std::get<0>(header);
        std::string header_value = std::get<1>(header);
        response->SetHeader(header_name.c_str(), header_value.c_str(), header_value.length(), true);
    }

    if (m_asgi_response_msg->content.length() == 0) {
        return kStepFinishRequest;
    }

    return kStepGotoNext;
}

std::unique_ptr<HttpRequestHandlerStep> WaitForResponseStep::GetNextStep()
{
    return std::make_unique<WriteResponseStep>(
        m_handler, std::move(m_asgi_response_msg)
    );
}


StepResult WriteResponseStep::Enter()
{
    // TODO: Handle streaming responses.

    IHttpResponse* response = m_http_context->GetResponse();

    BOOL completion_expected = FALSE;
    while (!completion_expected) {
        m_resp_chunk.DataChunkType = HttpDataChunkFromMemory;
        m_resp_chunk.FromMemory.pBuffer = (PVOID)(m_asgi_response_msg->content.c_str() + m_resp_bytes_written);
        m_resp_chunk.FromMemory.BufferLength = m_asgi_response_msg->content.length() - m_resp_bytes_written;

        DWORD bytes_written;
        HRESULT hr = response->WriteEntityChunks(
            &m_resp_chunk, 1,
            true, false, &bytes_written, &completion_expected
        );
        if (FAILED(hr)) {
            logger.debug() << "WriteEntityChunks() returned hr=" << hr;
            // TODO: Call some kind of Error();
            return kStepFinishRequest;
        }

        if (!completion_expected) {
            // Operation completed synchronously.
            auto result = OnAsyncCompletion(S_OK, bytes_written);
            // If we need to write more, we might as well do that here, rather than
            // yielding back to the request loop.
            if (result != kStepRerun) {
                return result;
            }
        }
    }

    return kStepAsyncPending;
}

StepResult WriteResponseStep::OnAsyncCompletion(HRESULT hr, DWORD num_bytes)
{
    // num_bytes is always 0, whether the operation completed sync or async.
    // The docs suggest this is becuase IIS has buffering enabled by default.
    // For now assume our data was sent correctly.
    // Revisit this when we handle streaming responses.
    return kStepFinishRequest;
}
