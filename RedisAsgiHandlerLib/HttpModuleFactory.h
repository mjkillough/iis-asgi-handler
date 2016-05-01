#pragma once

#include <memory>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>
#include <evntprov.h>


class HttpModuleFactory : public IHttpModuleFactory
{
public:
    HttpModuleFactory(const HTTP_MODULE_ID& module_id);
    virtual ~HttpModuleFactory();

    virtual HRESULT GetHttpModule(OUT CHttpModule** module, IN IModuleAllocator*);
    virtual void Terminate();

    void Log(const std::wstring& msg) const;

    const HTTP_MODULE_ID& module_id() const { return m_module_id; }

private:
    ::REGHANDLE m_etw_handle;
    HTTP_MODULE_ID m_module_id;
};
