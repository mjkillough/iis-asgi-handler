#pragma once

#include <memory>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "Logger.h"
#include "RedisChannelLayer.h"
#include "ResponsePump.h"


class HttpModuleFactory : public IHttpModuleFactory
{
public:
    HttpModuleFactory(const HTTP_MODULE_ID& module_id);
    virtual ~HttpModuleFactory();

    virtual HRESULT GetHttpModule(OUT CHttpModule** module, IN IModuleAllocator*);
    virtual void Terminate();


    const HTTP_MODULE_ID& module_id() const { return m_module_id; }


private:
    HTTP_MODULE_ID m_module_id;
    const EtwLogger logger; // must be declared before other members that rely on it.
    RedisChannelLayer m_channels;
    ResponsePump m_response_pump;
};
