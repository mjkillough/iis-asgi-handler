#define WIN32_LEAN_AND_MEAN
#include <ppltasks.h>

#include "HttpRequestHandler.h"
#include "HttpRequestHandlerSteps.h"
#include "ResponsePump.h"
#include "AsgiHttpResponseMsg.h"
#include "Logger.h"


namespace {

// TODO: Check these and see if http.sys gives us them anywhere - it must
//       have them! MJK did these on a train based on the values in http.h
// TODO: Lower case these. The ASGI spec specifies we should give headers
//       as lower case, so we may as well store them here that way.
std::unordered_map<USHORT, std::string> kKnownHeaderMap({
    { HttpHeaderCacheControl, "Cache-Control" },
    { HttpHeaderConnection, "Connection" },
    { HttpHeaderDate, "Date" },
    { HttpHeaderKeepAlive, "Keep-Alive" },
    { HttpHeaderPragma, "Pragma" },
    { HttpHeaderTrailer, "Trailer" },
    { HttpHeaderTransferEncoding, "Transfer-Encoding" },
    { HttpHeaderUpgrade, "Upgrade" },
    { HttpHeaderVia, "Via" },
    { HttpHeaderWarning, "Warning" },

    { HttpHeaderAllow, "Allow" },
    { HttpHeaderContentLength, "Content-Length" },
    { HttpHeaderContentType, "ContentType" },
    { HttpHeaderContentEncoding, "Content-Encoding" },
    { HttpHeaderContentLanguage, "Content-Language" },
    { HttpHeaderContentLocation, "Content-Location" },
    { HttpHeaderContentMd5, "Content-Md5" },
    { HttpHeaderContentRange, "Content-Range" },
    { HttpHeaderExpires, "Expires" },
    { HttpHeaderLastModified, "Last-Modified" },

    { HttpHeaderAccept, "Accept" },
    { HttpHeaderAcceptCharset, "Accept-Charset" },
    { HttpHeaderAcceptEncoding, "Accept-Encoding" },
    { HttpHeaderAcceptLanguage, "Accept-Language" },
    { HttpHeaderAuthorization, "Authorization" },
    { HttpHeaderCookie, "Cookie" },
    { HttpHeaderExpect, "Expect" },
    { HttpHeaderFrom, "From" },
    { HttpHeaderHost, "Host" },
    { HttpHeaderIfMatch, "If-Match" },

    { HttpHeaderIfModifiedSince, "If-Modified-Since" },
    { HttpHeaderIfNoneMatch, "If-None-Match" },
    { HttpHeaderIfRange, "If-Range" },
    { HttpHeaderIfUnmodifiedSince, "If-Unmodified-Since" },
    { HttpHeaderMaxForwards, "Max-Forwards" },
    { HttpHeaderProxyAuthorization, "Proxy-Authorization" },
    { HttpHeaderReferer, "Referer" },
    { HttpHeaderRange, "Range" },
    { HttpHeaderTe, "Te" },
    { HttpHeaderTranslate, "Translate" },

    { HttpHeaderUserAgent, "User-Agent" }
});

} // end anonymous namespace


HttpRequestHandler::HttpRequestHandler(
    ResponsePump& response_pump, RedisChannelLayer& channels, const Logger& logger, IHttpContext* http_context
)
    : m_response_pump(response_pump), m_channels(channels), m_logger(logger), m_http_context(http_context)
{
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::OnAcquireRequestState()
{
    IHttpRequest *request = m_http_context->GetRequest();
    HTTP_REQUEST *raw_request = request->GetRawHttpRequest();

    auto asgi_request_msg = std::make_unique<AsgiHttpRequestMsg>();
    asgi_request_msg->reply_channel = m_channels.NewChannel("http.request!");
    asgi_request_msg->http_version = GetRequestHttpVersion(request);
    asgi_request_msg->method = std::string(request->GetHttpMethod());
    asgi_request_msg->scheme = GetRequestScheme(raw_request);
    asgi_request_msg->path = GetRequestPath(raw_request);
    asgi_request_msg->query_string = GetRequestQueryString(raw_request);
    asgi_request_msg->root_path = ""; // TODO: Same as SCRIPT_NAME in WSGI. What r that?
    asgi_request_msg->headers = GetRequestHeaders(raw_request);
    asgi_request_msg->body.resize(request->GetRemainingEntityBytes());
    m_logger.Log(L"Reserved " + std::to_wstring(request->GetRemainingEntityBytes()));

    m_current_step = std::make_unique<ReadBodyStep>(*this, std::move(asgi_request_msg));

    auto step_name = std::string(typeid(*m_current_step.get()).name());
    auto step_name_w = std::wstring(step_name.begin(), step_name.end()); // XXX ugh!
    m_logger.Log(step_name_w + L"->Enter() being called");

    auto result = m_current_step->Enter();

    m_logger.Log(step_name_w + L"->Enter() = " + std::to_wstring(result));

    return HandlerStateMachine(result);
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::OnAsyncCompletion(IHttpCompletionInfo* completion_info)
{
    bool async_pending = false;
    HRESULT hr = completion_info->GetCompletionStatus();
    DWORD bytes = completion_info->GetCompletionBytes();

    auto step_name = std::string(typeid(*m_current_step.get()).name());
    auto step_name_w = std::wstring(step_name.begin(), step_name.end()); // XXX ugh!

    m_logger.Log(step_name_w + L"->OnAsyncCompletion() being called");

    auto result = m_current_step->OnAsyncCompletion(hr, bytes);

    m_logger.Log(step_name_w + L"->OnAsyncCompletion() = " + std::to_wstring(result));

    return HandlerStateMachine(result);
}

bool HttpRequestHandler::ReturnError(USHORT status, const std::string& reason)
{
    // TODO: Flush? Pass hr to SetStatus() to give better error message?
    IHttpResponse* response = m_http_context->GetResponse();
    response->Clear();
    response->SetStatus(status, reason.c_str());
    return false; // No async pending.
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::HandlerStateMachine(StepResult _result)
{
    StepResult result = _result;
    // This won't loop forever. We expect to return AsyncPending fairly often.
    while (true) {
        switch (result) {
        case kStepAsyncPending:
            return RQ_NOTIFICATION_PENDING;
        case kStepFinishRequest:
            return RQ_NOTIFICATION_FINISH_REQUEST;
        case kStepRerun: {
            auto step_name = std::string(typeid(*m_current_step.get()).name());
            auto step_name_w = std::wstring(step_name.begin(), step_name.end()); // XXX ugh!

            m_logger.Log(step_name_w + L"->Enter() being called");

            result = m_current_step->Enter();

            m_logger.Log(step_name_w + L"->Enter() = " + std::to_wstring(result));
        } break;
        case kStepGotoNext: {
            auto step_name = std::string(typeid(*m_current_step.get()).name());
            auto step_name_w = std::wstring(step_name.begin(), step_name.end()); // XXX ugh!

            m_logger.Log(step_name_w + L"->GetNextStep() being called");

            m_current_step = m_current_step->GetNextStep();

            step_name = std::string(typeid(*m_current_step.get()).name());
            step_name_w = std::wstring(step_name.begin(), step_name.end()); // XXX ugh!

            m_logger.Log(L"New step: " + step_name_w);
            m_logger.Log(step_name_w + L"->Enter() being called");

            result = m_current_step->Enter();

            m_logger.Log(step_name_w + L"->Enter() = " + std::to_wstring(result));
        } break;
        }
    }
}

std::string HttpRequestHandler::GetRequestHttpVersion(const IHttpRequest* request)
{
    USHORT http_ver_major, http_ver_minor;
    request->GetHttpVersion(&http_ver_major, &http_ver_minor);
    std::string http_version = std::to_string(http_ver_major);
    if (http_ver_major == 1) {
        http_version += "." + std::to_string(http_ver_minor);
    }
    return http_version;
}

std::string HttpRequestHandler::GetRequestScheme(const HTTP_REQUEST* raw_request)
{
    std::wstring url(
        raw_request->CookedUrl.pFullUrl,
        raw_request->CookedUrl.FullUrlLength / sizeof(wchar_t)
    );
    auto colon_idx = url.find(':');
    std::wstring scheme_w;
    // Don't assume the cooked URL has a scheme, although it should.
    if (colon_idx != url.npos) {
        scheme_w = url.substr(0, colon_idx);
    }
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(scheme_w);
}

std::string HttpRequestHandler::GetRequestPath(const HTTP_REQUEST* raw_request)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(std::wstring(
        raw_request->CookedUrl.pAbsPath,
        raw_request->CookedUrl.AbsPathLength / sizeof(wchar_t)
    ));
}

std::string HttpRequestHandler::GetRequestQueryString(const HTTP_REQUEST* raw_request)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(std::wstring(
        raw_request->CookedUrl.pQueryString,
        raw_request->CookedUrl.QueryStringLength / sizeof(wchar_t)
    ));
}

std::vector<std::tuple<std::string, std::string>> HttpRequestHandler::GetRequestHeaders(const HTTP_REQUEST* raw_request)
{
    std::vector<std::tuple<std::string, std::string>> headers;
    auto known_headers = raw_request->Headers.KnownHeaders;
    for (USHORT i = 0; i < HttpHeaderRequestMaximum; i++) {
        if (known_headers[i].RawValueLength > 0) {
            auto value = std::string(known_headers[i].pRawValue, known_headers[i].RawValueLength);
            headers.push_back(std::make_tuple(kKnownHeaderMap[i], value));
        }
    }

    // TODO: ASGI specifies headers should be lower-cases. Makes sense to do that here?
    auto unknown_headers = raw_request->Headers.pUnknownHeaders;
    for (USHORT i = 0; i < raw_request->Headers.UnknownHeaderCount; i++) {
        auto name = std::string(unknown_headers[i].pName, unknown_headers[i].NameLength);
        auto value = std::string(unknown_headers[i].pRawValue, unknown_headers[i].RawValueLength);
        headers.push_back(std::make_tuple(name, value));
    }

    return headers;
}
