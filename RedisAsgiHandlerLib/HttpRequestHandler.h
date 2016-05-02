#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "AsgiHttpRequestMsg.h"
#include "AsgiHttpResponseMsg.h"
#include "RedisChannelLayer.h"
#include "HttpModuleFactory.h"


class HttpRequestHandler : public IHttpStoredContext
{
public:
    HttpRequestHandler(const HttpModuleFactory& factory, IHttpContext* http_context);

    virtual void CleanupStoredContext()
    {
        delete this;
    }

    REQUEST_NOTIFICATION_STATUS OnAcquireRequestState(
        IHttpContext *http_context,
        IHttpEventProvider *provider
    );

    REQUEST_NOTIFICATION_STATUS OnAsyncCompletion(
        IHttpContext* http_context,
        DWORD notification,
        BOOL post_notification,
        IHttpEventProvider* provider,
        IHttpCompletionInfo* completion_info
    );

private:

    bool ReturnError(USHORT status = 500, const std::string& reason = "");

    enum RequestState {
        kStateInitial,
        kStateReadingBody,
        kStateSendingToApplication,
        // kStateWaitingForResponse,
        // kStateSendingResponse,
        // kStateWaitingForMoreResponse,
        kStateComplete,
    };

    // These return whether an async operation is pending.
    bool ReadBodyAsync();
    bool OnReadingBodyAsyncComplete(IHttpCompletionInfo* completion_info);
    bool SendToApplication();
    // bool OnSendToApplicationAsyncComplete();
    // bool WaitForResponseAsync();
    // bool OnWaitForResponseAsyncComplete();
    // bool SendResponseAsync();
    // bool OnSendingResponseAsyncComplete(IHttpCompletionInfo* completion_info);

    static std::string GetRequestHttpVersion(const IHttpRequest* request);
    static std::string GetRequestScheme(const HTTP_REQUEST* raw_request);
    static std::string GetRequestPath(const HTTP_REQUEST* raw_request);
    static std::string GetRequestQueryString(const HTTP_REQUEST* raw_request);
    static std::vector<std::tuple<std::string, std::string>> GetRequestHeaders(const HTTP_REQUEST* raw_request);

    IHttpContext* m_http_context;

    // State for kStateReadingBody.
    DWORD m_body_bytes_read;

    const HttpModuleFactory& m_factory;
    struct AsgiHttpRequestMsg m_asgi_request_msg;
    struct AsgiHttpResponseMsg m_asgi_response_msg;
    RedisChannelLayer m_channels;
    RequestState m_request_state;
};
