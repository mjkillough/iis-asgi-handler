#include "HttpModuleFactory.h"

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"


namespace {
    // {B057F98C-CB95-413D-AFAE-8ED010DB73C5}
    static const GUID EtwGuid = { 0xb057f98c, 0xcb95, 0x413d,{ 0xaf, 0xae, 0x8e, 0xd0, 0x10, 0xdb, 0x73, 0xc5 } };
} // end anonymous namespace

HttpModuleFactory::HttpModuleFactory()
{
    ::EventRegister(&EtwGuid, nullptr, nullptr, &m_etw_handle);
}

HttpModuleFactory::~HttpModuleFactory()
{
    ::EventUnregister(m_etw_handle);
}

HRESULT HttpModuleFactory::GetHttpModule(OUT CHttpModule ** module, IN IModuleAllocator *)
{
    *module = new HttpModule(*this);
    return S_OK;
}

void HttpModuleFactory::Terminate()
{
}

void HttpModuleFactory::Log(const std::wstring& msg) const
{
    ::EventWriteString(m_etw_handle, 0, 0, msg.c_str());
}
