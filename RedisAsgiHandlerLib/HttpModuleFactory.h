#pragma once

#include <memory>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <evntprov.h>


class HttpModuleFactory : public IHttpModuleFactory
{
public:
    HttpModuleFactory();
    virtual ~HttpModuleFactory();

    virtual HRESULT GetHttpModule(OUT CHttpModule** module, IN IModuleAllocator*);
    virtual void Terminate();

    void Log(const std::wstring& msg) const;

private:
    ::REGHANDLE m_etw_handle;
};
