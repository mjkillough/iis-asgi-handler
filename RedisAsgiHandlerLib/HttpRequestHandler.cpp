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


HttpRequestHandler::HttpRequestHandler()
{
}

REQUEST_NOTIFICATION_STATUS HttpRequestHandler::OnAcquireRequestState(
    IHttpContext* http_context, IHttpEventProvider* provider
)
{
    IHttpRequest *request = http_context->GetRequest();
    HTTP_REQUEST *raw_request = request->GetRawHttpRequest();
    IHttpResponse *response = http_context->GetResponse();

    m_asgi_request_msg.reply_channel = m_channels.NewChannel("http.request!");
    m_asgi_request_msg.http_version = GetRequestHttpVersion(request);
    m_asgi_request_msg.method = std::string(request->GetHttpMethod());
    m_asgi_request_msg.scheme = GetRequestScheme(raw_request);
    m_asgi_request_msg.path = GetRequestPath(raw_request);
    m_asgi_request_msg.query_string = GetRequestQueryString(raw_request);
    m_asgi_request_msg.root_path = ""; // TODO: Same as SCRIPT_NAME in WSGI. What r that?
    m_asgi_request_msg.headers = GetRequestHeaders(raw_request);

    // TODO: Consider writing the body directly to the msgpack object? We are copying around a lot.
    // TODO: Split the body into chunks?
    // TODO: Use async IO.
    // TODO: Timeout if we don't get enough data.
    DWORD content_length = request->GetRemainingEntityBytes();
    m_asgi_request_msg.body.resize(content_length);
    DWORD bytes_received = 0;
    while (bytes_received < content_length) {
        request->ReadEntityBody(
            m_asgi_request_msg.body.data() + bytes_received,
            content_length - bytes_received,
            false, &bytes_received, nullptr
        );
    }

    m_channels.Send("http.request", m_asgi_request_msg);

    RedisData data = m_channels.Receive(m_asgi_request_msg.reply_channel);
    msgpack::object_handle response_msg_handle = msgpack::unpack(data.get(), data.length());
    AsgiHttpResponseMsg response_msg = response_msg_handle.get().as<AsgiHttpResponseMsg>();

    response->Clear();
    response->SetStatus(response_msg.status, ""); // Can we pass nullptr for the reason?
    DWORD bytes_sent = 0;
    // WriteEntityChunks doesn't seem to return bytes_sent?
    // while (bytes_sent < response_msg.content.length()) {
    HTTP_DATA_CHUNK chunk;
    chunk.DataChunkType = HttpDataChunkFromMemory;
    chunk.FromMemory.pBuffer = (PVOID)(response_msg.content.c_str() + bytes_sent);
    chunk.FromMemory.BufferLength = response_msg.content.length() - bytes_sent;
    response->WriteEntityChunks(&chunk, 1, false, false, &bytes_sent, nullptr);
    // }
    for (auto header : response_msg.headers) {
        std::string header_name = std::get<0>(header);
        std::string header_value = std::get<1>(header);
        response->SetHeader(header_name.c_str(), header_value.c_str(), header_value.length(), true);
    }

    return RQ_NOTIFICATION_FINISH_REQUEST;
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
