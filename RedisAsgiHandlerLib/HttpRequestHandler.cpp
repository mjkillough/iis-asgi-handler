#include <ppltasks.h>

#include "HttpRequestHandler.h"

#include "AsgiHttpResponseMsg.h"


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


HttpRequestHandler::HttpRequestHandler(const HttpModuleFactory& factory, IHttpContext* http_context)
    : m_factory(factory), m_http_context(http_context),
      m_request_state(kStateInitial), m_body_bytes_read(0),
      m_resp_bytes_written(0)
{
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::OnAcquireRequestState(
    IHttpContext* http_context, IHttpEventProvider* provider
)
{
    // TODO: Assert we are in kStateInitial.

    IHttpRequest *request = http_context->GetRequest();
    HTTP_REQUEST *raw_request = request->GetRawHttpRequest();

    m_asgi_request_msg.reply_channel = m_channels.NewChannel("http.request!");
    m_asgi_request_msg.http_version = GetRequestHttpVersion(request);
    m_asgi_request_msg.method = std::string(request->GetHttpMethod());
    m_asgi_request_msg.scheme = GetRequestScheme(raw_request);
    m_asgi_request_msg.path = GetRequestPath(raw_request);
    m_asgi_request_msg.query_string = GetRequestQueryString(raw_request);
    m_asgi_request_msg.root_path = ""; // TODO: Same as SCRIPT_NAME in WSGI. What r that?
    m_asgi_request_msg.headers = GetRequestHeaders(raw_request);
    m_asgi_request_msg.body.resize(request->GetRemainingEntityBytes());

    m_factory.Log(L"About to ReadBodyAsync().");
    bool async_pending = ReadBodyAsync();
    if (async_pending) {
        m_factory.Log(L"ReadBodyAsync() returned true -- async operation pending.");
        return RQ_NOTIFICATION_PENDING;
    }

    m_request_state = kStateComplete;
    return RQ_NOTIFICATION_FINISH_REQUEST;
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::OnAsyncCompletion(
    IHttpContext* http_context, DWORD notification, BOOL post_notification,
    IHttpEventProvider* provider, IHttpCompletionInfo* completion_info
)
{
    bool async_pending = false;
    HRESULT hr = completion_info->GetCompletionStatus();
    DWORD bytes = completion_info->GetCompletionBytes();

    switch (m_request_state) {
    case kStateReadingBody:
        async_pending = OnReadingBodyAsyncComplete(hr, bytes);
        break;
    case kStateWritingResponse:
        async_pending = OnWriteResponseAsyncComplete(hr, bytes);
        break;
    case kStateSendingToApplication:
        async_pending = OnSendToApplicationAsyncComplete();
        break;
    default:
        m_factory.Log(L"OnAsyncCompletion() called whilst in unexpected state: " + std::to_wstring(m_request_state));
        async_pending = ReturnError();
        break;
    }

    if (async_pending) {
        m_factory.Log(L"OnAsyncCompletion() returning RQ_NOTIFICATION_PENDING.");
        return RQ_NOTIFICATION_PENDING;
    }

    m_request_state = kStateComplete;
    return RQ_NOTIFICATION_FINISH_REQUEST;
}

bool HttpRequestHandler::ReturnError(USHORT status, const std::string& reason)
{
    // TODO: Flush? Pass hr to SetStatus() to give better error message?
    IHttpResponse* response = m_http_context->GetResponse();
    response->Clear();
    response->SetStatus(status, reason.c_str());
    return false; // No async pending.
}

bool HttpRequestHandler::ReadBodyAsync()
{
    IHttpRequest* request = m_http_context->GetRequest();

    // TODO: Consider writing the body directly to the msgpack object?
    //       We are copying around a lot.
    // TODO: Split the body into chunks?
    // TODO: Pass completion_expected to handle async completion inline

    DWORD remaining_bytes = request->GetRemainingEntityBytes();
    if (remaining_bytes == 0) {
        return SendToApplication();
    }

    m_request_state = kStateReadingBody;

    DWORD bytes_read;
    BOOL completion_expected;
    HRESULT hr = request->ReadEntityBody(
        m_asgi_request_msg.body.data() + m_body_bytes_read, remaining_bytes,
        true, &bytes_read, &completion_expected
    );
    if (FAILED(hr)) {
        m_factory.Log(L"ReadEntityBody returned hr=" + std::to_wstring(hr));
        return ReturnError();
    }
    if (!completion_expected) {
        // Operation completed synchronously.
        return OnReadingBodyAsyncComplete(S_OK, bytes_read);
    }

    return true; // async pending
}

bool HttpRequestHandler::OnReadingBodyAsyncComplete(HRESULT hr, DWORD bytes_read)
{
    if (FAILED(hr)) {
        m_factory.Log(L"OnReadyBodyingAsyncComplete found GetCompletionStatus()=" + std::to_wstring(hr));
        return ReturnError();
    }

    IHttpRequest* request = m_http_context->GetRequest();
    m_body_bytes_read += bytes_read;
    if (request->GetRemainingEntityBytes()) {
        return ReadBodyAsync();
    }

    return SendToApplication();
}

bool HttpRequestHandler::SendToApplication()
{
    m_request_state = kStateSendingToApplication;

    auto task = m_channels.Send("http.request", m_asgi_request_msg);
    task.then([this]() {
        m_http_context->PostCompletion(0);
        m_factory.Log(L"m_channels.Send() task completed; PostCompletion() called.");
    });

    return true; // async operation pending
}

bool HttpRequestHandler::OnSendToApplicationAsyncComplete()
{
    IHttpResponse *response = m_http_context->GetResponse();

    RedisData data = m_channels.Receive(m_asgi_request_msg.reply_channel);
    msgpack::object_handle response_msg_handle = msgpack::unpack(data.get(), data.length());
    m_asgi_response_msg = response_msg_handle.get().as<AsgiHttpResponseMsg>();

    response->Clear();
    response->SetStatus(m_asgi_response_msg.status, ""); // Can we pass nullptr for the reason?
    for (auto header : m_asgi_response_msg.headers) {
        std::string header_name = std::get<0>(header);
        std::string header_value = std::get<1>(header);
        response->SetHeader(header_name.c_str(), header_value.c_str(), header_value.length(), true);
    }

    if (m_asgi_response_msg.content.length() > 0) {
        return WriteResponseAsync();
    }

    return false;
}

bool HttpRequestHandler::WriteResponseAsync()
{
    // TODO: Handle streaming responses.

    m_request_state = kStateWritingResponse;

    IHttpResponse* response = m_http_context->GetResponse();

    m_resp_chunk.DataChunkType = HttpDataChunkFromMemory;
    m_resp_chunk.FromMemory.pBuffer = (PVOID)(m_asgi_response_msg.content.c_str() + m_resp_bytes_written);
    m_resp_chunk.FromMemory.BufferLength = m_asgi_response_msg.content.length() - m_resp_bytes_written;

    DWORD bytes_written;
    BOOL completion_expected;
    HRESULT hr = response->WriteEntityChunks(
        &m_resp_chunk, 1,
        true, false, &bytes_written, &completion_expected
    );
    if (FAILED(hr)) {
        m_factory.Log(L"WriteEntityChunks returned hr=" + std::to_wstring(hr));
        return ReturnError();
    }
    if (!completion_expected) {
        // Operation completed synchronously.
        return OnWriteResponseAsyncComplete(S_OK, bytes_written);
    }

    return false; // async operation pending
}

bool HttpRequestHandler::OnWriteResponseAsyncComplete(HRESULT hr, DWORD bytes_read)
{
    // TODO: Handle streaming responses.
    return false; // no async operation pending; we're done.
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
