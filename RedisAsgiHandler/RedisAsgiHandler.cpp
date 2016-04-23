#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "HttpModuleFactory.h"

HRESULT __stdcall RegisterModule(
    DWORD iisVersion, IHttpModuleRegistrationInfo* moduleInfo, IHttpServer* httpServer)
{
    // MSDN reckons we should store moduleInfo->GetId() and httpServer 
    // somewhere global.

    auto factory = new HttpModuleFactory();
    HRESULT hr = moduleInfo->SetRequestNotifications(
        factory, RQ_ACQUIRE_REQUEST_STATE, 0
    );

	return S_OK;
}