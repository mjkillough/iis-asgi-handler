#include "GlobalModule.h"

#include <vector>
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "ScopedResources.h"


namespace {
    // {8FC896E8-4370-4976-855B-19B70C976414}
    static const GUID logger_etw_guid = { 0x8fc896e8, 0x4370, 0x4976,{ 0x85, 0x5b, 0x19, 0xb7, 0xc, 0x97, 0x64, 0x14 } };
}


GlobalModule::GlobalModule(IHttpServer *http_server)
    : logger(logger_etw_guid), m_http_server(http_server)
{
    logger.debug() << "Constructed GlobalModule";
}


GLOBAL_NOTIFICATION_STATUS GlobalModule::OnGlobalApplicationStart(
    IHttpApplicationStartProvider* provider
)
{
    auto app_name = std::wstring{ provider->GetApplication()->GetApplicationId() };
    logger.debug() << "OnGlobalApplicationStart: " << app_name;

    try {
        LoadConfiguration(provider->GetApplication());
    } catch (const std::runtime_error& e) {
        logger.debug() << "Error loading configuration for " << app_name << ": "
                       << "";
    }

    return GL_NOTIFICATION_CONTINUE;
}


GLOBAL_NOTIFICATION_STATUS GlobalModule::OnGlobalApplicationStop(
    IHttpApplicationStopProvider* provider
)
{
    auto app_name = std::wstring{ provider->GetApplication()->GetApplicationId() };
    logger.debug() << "OnGlobalApplicationStop: " << app_name << " "
                   << "Terminating running pools.";

    // Destroy all our pools, which has the side-effect of terminating
    // their processes.
    m_pools.clear();

    logger.debug() << "OnGlobalApplicationStop finished";

    return GL_NOTIFICATION_CONTINUE;
}


#define RAISE_ON_FAILURE(code, msg)         \
    {                                       \
        auto __hr = HRESULT{ code };        \
        if (FAILED(__hr)) {                 \
            throw std::runtime_error(msg);  \
        }                                   \
    }                                       \

void GlobalModule::LoadConfiguration(IHttpApplication *application)
{
    auto section_name = ScopedBstr{ L"system.webServer/processPools" };
    auto config_path = ScopedBstr{ application->GetAppConfigPath() };

    auto section = ScopedConfig<IAppHostElement>{};
    auto hr = m_http_server->GetAdminManager()->GetAdminSection(
        section_name.Get(), config_path.Get(), section.Receive()
    );
    if (FAILED(hr)) {
        // The schema is probably not installed. When we have logging,
        // we should log an error, but shouldn't crash the server..
        logger.debug() << "Could not open system.webServer/processPools. "
                       << "Is the config schema installed?";
        return;
    }

    auto collection = ScopedConfig<IAppHostElementCollection>{};
    RAISE_ON_FAILURE(section->get_Collection(collection.Receive()), "get_Collection()");

    auto element_count = DWORD{ 0 };
    RAISE_ON_FAILURE(collection->get_Count(&element_count), "get_Count()");

    for (auto element_idx = 0; element_idx < element_count; element_idx++) {
        auto element_variant = VARIANT{ 0 };
        element_variant.vt = VT_I4;
        element_variant.lVal = element_idx;

        auto element = ScopedConfig<IAppHostElement>{};
        RAISE_ON_FAILURE(collection->get_Item(element_variant, element.Receive()), "get_Item()");

        // Each element should be a <processPool/>, which will have the following properties:
        auto executable = GetProperty(element.Get(), L"executable");
        auto arguments = GetProperty(element.Get(), L"arguments");

        // Create a ProcessPool for each:
        m_pools.push_back(std::make_unique<ProcessPool>(logger, executable, arguments, 1));
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
