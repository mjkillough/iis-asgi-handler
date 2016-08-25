#include "GlobalModule.h"

#include <vector>
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "ScopedResources.h"


GlobalModule::GlobalModule(IHttpServer *http_server)
    : m_http_server(http_server)
{
}


GLOBAL_NOTIFICATION_STATUS GlobalModule::OnGlobalApplicationStart(
    IHttpApplicationStartProvider* provider
)
{
    LoadConfiguration(provider->GetApplication());

    return GL_NOTIFICATION_CONTINUE;
}


GLOBAL_NOTIFICATION_STATUS GlobalModule::OnGlobalApplicationStop(
    IHttpApplicationStopProvider* provider
)
{
    // Destroy all our pools, which has the side-effect of terminating
    // their processes.
    m_pools.clear();

    return GL_NOTIFICATION_CONTINUE;
}


#define RAISE_ON_FAILURE(code, msg)         \
    {                                       \
        HRESULT __hr = code;                \
        if (FAILED(__hr)) {                 \
            throw std::runtime_error(msg);  \
        }                                   \
    }                                       \

void GlobalModule::LoadConfiguration(IHttpApplication *application)
{
    auto section_name = ScopedBstr{ L"system.webServer/processPools" };
    auto config_path = ScopedBstr{ application->GetAppConfigPath() };

    ScopedConfig<IAppHostElement> section;
    auto hr = m_http_server->GetAdminManager()->GetAdminSection(
        section_name.Get(), config_path.Get(), section.Receive()
    );
    if (FAILED(hr)) {
        // The schema is probably not installed. When we have logging,
        // we should log an error, but shouldn't crash the server..
        return;
    }

    ScopedConfig<IAppHostElementCollection> collection;
    RAISE_ON_FAILURE(section->get_Collection(collection.Receive()), "get_Collection()");

    DWORD element_count = 0;
    RAISE_ON_FAILURE(collection->get_Count(&element_count), "get_Count()");

    for (DWORD element_idx = 0; element_idx < element_count; element_idx++) {
        VARIANT element_variant = { 0 };
        element_variant.vt = VT_I4;
        element_variant.lVal = element_idx;

        ScopedConfig<IAppHostElement> element;
        RAISE_ON_FAILURE(collection->get_Item(element_variant, element.Receive()), "get_Item()");

        // Each element should be a <processPool/>, which will have the following properties:
        auto executable = GetProperty(element.Get(), L"executable");
        auto arguments = GetProperty(element.Get(), L"arguments");

        // Create a ProcessPool for each:
        m_pools.push_back(ProcessPool{ executable, arguments });
    }
}

std::wstring GlobalModule::GetProperty(IAppHostElement *element, std::wstring name)
{
    auto bstr_name = ScopedBstr{ name.c_str() };
    ScopedConfig<IAppHostProperty> property;
    RAISE_ON_FAILURE(element->GetPropertyByName(
        bstr_name.Get(), property.Receive()
    ), "GetPropertyByName()");

    BSTR bstr_value = nullptr;
    RAISE_ON_FAILURE(property->get_StringValue(&bstr_value), "get_StringValue()");

    return std::wstring(bstr_value);
}
