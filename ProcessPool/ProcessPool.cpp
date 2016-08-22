#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "GlobalModule.h"


HRESULT __stdcall RegisterModule(
    DWORD iis_version, IHttpModuleRegistrationInfo* module_info, IHttpServer* http_server
)
{
    auto module = new GlobalModule();
    auto hr = module_info->SetGlobalNotifications(
        module,
        GL_APPLICATION_START | GL_APPLICATION_STOP
    );
	return hr;
}