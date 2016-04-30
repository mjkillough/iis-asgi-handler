#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModuleFactory.h"


class HttpModule : public CHttpModule
{
public:
    HttpModule(const HttpModuleFactory& factory);

    REQUEST_NOTIFICATION_STATUS OnAcquireRequestState(
        IN IHttpContext* httpContext, IN OUT IHttpEventProvider* provider
    );

private:
    const HttpModuleFactory& m_factory;
};
