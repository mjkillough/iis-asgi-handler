#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "AsgiHttpRequestMsg.h"
#include "RedisChannelLayer.h"



class HttpRequestHandler : public IHttpStoredContext
{
public:
    HttpRequestHandler();

    virtual void CleanupStoredContext()
    {
        delete this;
    }

    REQUEST_NOTIFICATION_STATUS OnAcquireRequestState(
        IHttpContext *http_context,
        IHttpEventProvider *provider
    );

public:
    static std::string GetRequestHttpVersion(const IHttpRequest* request);
    static std::string GetRequestScheme(const HTTP_REQUEST* raw_request);
    static std::string GetRequestPath(const HTTP_REQUEST* raw_request);
    static std::string GetRequestQueryString(const HTTP_REQUEST* raw_request);
    static std::vector<std::tuple<std::string, std::string>> GetRequestHeaders(const HTTP_REQUEST* raw_request);

    struct AsgiHttpRequestMsg m_asgi_request_msg;
    RedisChannelLayer m_channels;
};
