#pragma once

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>


class HttpModuleFactory : public IHttpModuleFactory
{
public:

    virtual HRESULT GetHttpModule(OUT CHttpModule** module, IN IModuleAllocator*);
    virtual void Terminate();
};