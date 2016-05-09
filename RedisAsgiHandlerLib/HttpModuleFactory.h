#pragma once

#include <memory>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "ResponsePump.h"
#include "Logger.h"


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
    Logger logger; // must be declared before other members that rely on it.
    ResponsePump m_response_pump;
};
