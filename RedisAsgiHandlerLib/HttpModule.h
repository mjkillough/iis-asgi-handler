#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>


class HttpModule : public CHttpModule
{
public:

    REQUEST_NOTIFICATION_STATUS OnAcquireRequestState(
        IN IHttpContext* httpContext, IN OUT IHttpEventProvider * provider
    );

private:
};
