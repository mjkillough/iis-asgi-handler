#include "HttpModuleFactory.h"

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModule.h"


HRESULT HttpModuleFactory::GetHttpModule(OUT CHttpModule ** module, IN IModuleAllocator *)
{
    *module = new HttpModule();
    return S_OK;
}

void HttpModuleFactory::Terminate()
{
}
