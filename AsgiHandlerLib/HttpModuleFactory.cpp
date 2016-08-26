#include "HttpModuleFactory.h"

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"


namespace {
    // {B057F98C-CB95-413D-AFAE-8ED010DB73C5}
    static const GUID logger_etw_guid = { 0xb057f98c, 0xcb95, 0x413d,{ 0xaf, 0xae, 0x8e, 0xd0, 0x10, 0xdb, 0x73, 0xc5 } };
}


HttpModuleFactory::HttpModuleFactory(const HTTP_MODULE_ID& module_id)
    : logger(logger_etw_guid), m_module_id(module_id),
      m_response_pump(logger, m_channels)
{
    logger.debug() << "Creating HttpModuleFactory";
    m_response_pump.Start();
}

HttpModuleFactory::~HttpModuleFactory()
{
    logger.debug() << "Destroying HttpModuleFactory";
}

HRESULT HttpModuleFactory::GetHttpModule(OUT CHttpModule ** module, IN IModuleAllocator *)
{
    // This is called once for each request. We may want to return the same HttpModule
    // to multiple requests, given that the actual per-request state is stored elsewhere.
    *module = new HttpModule(*this, m_response_pump, logger);
    return S_OK;
}

void HttpModuleFactory::Terminate()
{
}
