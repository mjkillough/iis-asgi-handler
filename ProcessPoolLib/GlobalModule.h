#pragma once

#include <memory>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "ProcessPool.h"


class GlobalModule : public CGlobalModule
{
public:
    GlobalModule(IHttpServer *http_server);

    virtual void Terminate() { delete this; }

    virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStart(
        IHttpApplicationStartProvider* provider
    );
    virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStop(
        IHttpApplicationStopProvider* provider
    );

protected:
    void LoadConfiguration(IHttpApplication *application);
    std::wstring GetProperty(IAppHostElement *element, std::wstring name);

private:
    IHttpServer *m_http_server;
    std::vector<ProcessPool> m_pools;
};
