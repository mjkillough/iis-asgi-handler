#pragma once

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "ProcessPool.h"


class GlobalModule : public CGlobalModule
{
public:
    virtual void Terminate() { delete this; }

    virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStart(
        IHttpApplicationStartProvider* provider
    );
    virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStop(
        IHttpApplicationStopProvider* provider
    );

private:
    std::unique_ptr<ProcessPool> m_pool;
};
