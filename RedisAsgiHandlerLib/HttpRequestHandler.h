#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "AsgiHttpRequestMsg.h"
#include "AsgiHttpResponseMsg.h"
#include "HttpModuleFactory.h"
#include "ResponsePump.h"
#include "Logger.h"


class HttpRequestHandler : public IHttpStoredContext
{
public:
    HttpRequestHandler(
        const HttpModuleFactory& factory, ResponsePump& response_pump,
        const Logger& logger, IHttpContext* http_context
    );

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
        kStateWaitingForResponse,
        kStateWritingResponse,
        // kStateWaitingForMoreResponse,
        kStateComplete,
    };

    // These return whether an async operation is pending.
    bool ReadBodyAsync();
    bool OnReadingBodyAsyncComplete(HRESULT hr, DWORD bytes_read);
    bool SendToApplication();
    bool OnSendToApplicationAsyncComplete();
    bool WaitForResponseAsync();
    bool OnWaitForResponseAsyncComplete();
    bool WriteResponseAsync();
    bool OnWriteResponseAsyncComplete(HRESULT hr, DWORD bytes_read);

    static std::string GetRequestHttpVersion(const IHttpRequest* request);
    static std::string GetRequestScheme(const HTTP_REQUEST* raw_request);
    static std::string GetRequestPath(const HTTP_REQUEST* raw_request);
    static std::string GetRequestQueryString(const HTTP_REQUEST* raw_request);
    static std::vector<std::tuple<std::string, std::string>> GetRequestHeaders(const HTTP_REQUEST* raw_request);

    IHttpContext* m_http_context;

    // State for kStateReadingBody.
    DWORD m_body_bytes_read;
    // State for kStateWritingResponse
    HTTP_DATA_CHUNK m_resp_chunk;
    DWORD m_resp_bytes_written;
    // State for kStateWaitingForResponse/kStateWritingResponse
    AsgiHttpResponseMsg m_asgi_response_msg;

    const HttpModuleFactory& m_factory;
    ResponsePump& m_response_pump;
    const Logger& m_logger;
    AsgiHttpRequestMsg m_asgi_request_msg;
    RedisChannelLayer m_channels;
    RequestState m_request_state;
};
