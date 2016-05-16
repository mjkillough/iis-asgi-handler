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
#include "RequestHandler.h"


class ReadBodyStep : public RequestHandlerStep
{
public:
    ReadBodyStep(
        RequestHandler& handler, std::unique_ptr<AsgiHttpRequestMsg>& asgi_request_msg
    ) : RequestHandlerStep(handler), m_asgi_request_msg(std::move(asgi_request_msg)), m_body_bytes_read(0)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();

protected:
    std::unique_ptr<AsgiHttpRequestMsg> m_asgi_request_msg;
    DWORD m_body_bytes_read;
};


class SendToApplicationStep : public RequestHandlerStep
{
public:
    SendToApplicationStep(
        RequestHandler& handler, std::unique_ptr<AsgiHttpRequestMsg>& asgi_request_msg
    ) : RequestHandlerStep(handler), m_asgi_request_msg(std::move(asgi_request_msg))
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();

protected:
    std::unique_ptr<AsgiHttpRequestMsg> m_asgi_request_msg;
};


class WaitForResponseStep : public RequestHandlerStep
{
public:
    WaitForResponseStep(
        RequestHandler& handler, const std::string& reply_channel
    ) : RequestHandlerStep(handler), m_reply_channel(reply_channel)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);
    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();

protected:
    const std::string m_reply_channel;
    std::unique_ptr<AsgiHttpResponseMsg> m_asgi_response_msg;
};


class WriteResponseStep : public RequestHandlerStep
{
public:
    WriteResponseStep(
        RequestHandler& handler, std::unique_ptr<AsgiHttpResponseMsg>& asgi_response_msg,
        std::string reply_channel
    ) : RequestHandlerStep(handler), m_asgi_response_msg(std::move(asgi_response_msg)),
        m_reply_channel(reply_channel), m_resp_bytes_written(0)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);

    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();

protected:
    std::unique_ptr<AsgiHttpResponseMsg> m_asgi_response_msg;
    const std::string m_reply_channel;
    HTTP_DATA_CHUNK m_resp_chunk;
    DWORD m_resp_bytes_written;
};


class FlushResponseStep : public RequestHandlerStep
{
public:
    FlushResponseStep(
        RequestHandler& handler, std::string reply_channel
    ) : RequestHandlerStep(handler), m_reply_channel(reply_channel)
    { }

    virtual StepResult Enter();
    virtual StepResult OnAsyncCompletion(HRESULT hr, DWORD num_bytes);

    virtual std::unique_ptr<RequestHandlerStep> GetNextStep();

protected:
    const std::string m_reply_channel;
};
