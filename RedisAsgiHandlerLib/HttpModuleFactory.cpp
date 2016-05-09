#include "HttpModuleFactory.h"

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"


HttpModuleFactory::HttpModuleFactory(const HTTP_MODULE_ID& module_id)
    : m_module_id(module_id), m_response_pump(logger)
{
}

HttpModuleFactory::~HttpModuleFactory()
{
}

HRESULT HttpModuleFactory::GetHttpModule(OUT CHttpModule ** module, IN IModuleAllocator *)
{
    *module = new HttpModule(*this, m_response_pump, logger);
    return S_OK;
}

void HttpModuleFactory::Terminate()
{
}
