#pragma once

#include <stdexcept>
#include <typeinfo>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "AsgiHttpRequestMsg.h"
#include "AsgiHttpResponseMsg.h"
#include "Logger.h"
#include "RedisChannelLayer.h"
#include "ResponsePump.h"


class HttpRequestHandler;

enum StepResult {
    kStepAsyncPending,
    kStepRerun, // Re-Enter()s the same state.
    kStepGotoNext, // Causes us to call GetNextStep().
    kStepFinishRequest
};


// Implementations defined in HttpRequestHandlerSteps.{cpp,h}
class HttpRequestHandlerStep
{
public:
    HttpRequestHandlerStep(HttpRequestHandler& handler);

    virtual StepResult Enter() = 0;
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes) = 0;
    virtual std::unique_ptr<HttpRequestHandlerStep> GetNextStep() = 0;

protected:
    HttpRequestHandler& m_handler;
    // Expose some of m_handler's protected members here, so that they are
    // accessible from Step subclasses.
    const Logger& logger;
    IHttpContext* m_http_context;
    ResponsePump& m_response_pump;
    RedisChannelLayer& m_channels;
};


class ReadBodyStep : public HttpRequestHandlerStep
{
public:
    ReadBodyStep(
        HttpRequestHandler& handler, std::unique_ptr<AsgiHttpRequestMsg>& asgi_request_msg
    ) : HttpRequestHandlerStep(handler), m_asgi_request_msg(std::move(asgi_request_msg)), m_body_bytes_read(0)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<HttpRequestHandlerStep> GetNextStep();

protected:
    std::unique_ptr<AsgiHttpRequestMsg> m_asgi_request_msg;
    DWORD m_body_bytes_read;
};


class SendToApplicationStep : public HttpRequestHandlerStep
{
public:
    SendToApplicationStep(
        HttpRequestHandler& handler, std::unique_ptr<AsgiHttpRequestMsg>& asgi_request_msg
    ) : HttpRequestHandlerStep(handler), m_asgi_request_msg(std::move(asgi_request_msg))
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<HttpRequestHandlerStep> GetNextStep();

protected:
    std::unique_ptr<AsgiHttpRequestMsg> m_asgi_request_msg;
};


class WaitForResponseStep : public HttpRequestHandlerStep
{
public:
    WaitForResponseStep(
        HttpRequestHandler& handler, const std::string& reply_channel
    ) : HttpRequestHandlerStep(handler), m_reply_channel(reply_channel)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<HttpRequestHandlerStep> GetNextStep();

protected:
    const std::string m_reply_channel;
    std::unique_ptr<AsgiHttpResponseMsg> m_asgi_response_msg;
};


class WriteResponseStep : public HttpRequestHandlerStep
{
public:
    WriteResponseStep(
        HttpRequestHandler& handler, std::unique_ptr<AsgiHttpResponseMsg>& asgi_response_msg,
        std::string reply_channel
    ) : HttpRequestHandlerStep(handler), m_asgi_response_msg(std::move(asgi_response_msg)),
        m_reply_channel(reply_channel), m_resp_bytes_written(0)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);

    virtual std::unique_ptr<HttpRequestHandlerStep> GetNextStep();

protected:
    std::unique_ptr<AsgiHttpResponseMsg> m_asgi_response_msg;
    const std::string m_reply_channel;
    HTTP_DATA_CHUNK m_resp_chunk;
    DWORD m_resp_bytes_written;
};


class FlushResponseStep : public HttpRequestHandlerStep
{
public:
    FlushResponseStep(
        HttpRequestHandler& handler, std::string reply_channel
    ) : HttpRequestHandlerStep(handler), m_reply_channel(reply_channel)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);

    virtual std::unique_ptr<HttpRequestHandlerStep> GetNextStep();

protected:
    const std::string m_reply_channel;
};
