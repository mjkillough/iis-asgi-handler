#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModuleFactory.h"


HRESULT __stdcall RegisterModule(
    DWORD iisVersion, IHttpModuleRegistrationInfo* moduleInfo, IHttpServer* httpServer)
{
    auto factory = new HttpModuleFactory(moduleInfo->GetId());
    HRESULT hr = moduleInfo->SetRequestNotifications(
        factory, RQ_EXECUTE_REQUEST_HANDLER, 0
    );

	return S_OK;
}