#pragma once

#include <memory>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "ProcessPool.h"
#include "Logger.h"


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
    std::wstring GetProperty(IAppHostElement *element, const std::wstring& name);

private:
    const EtwLogger logger;
    IHttpServer *m_http_server;
    std::vector<std::unique_ptr<ProcessPool>> m_pools;
};
