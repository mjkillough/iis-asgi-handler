#define WIN32_LEAN_AND_MEAN
#include <httpserv.h>

#include "RequestHandler.h"


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


RequestHandlerStep::RequestHandlerStep(RequestHandler& handler)
    : m_handler(handler), m_http_context(handler.m_http_context),
      logger(handler.logger), m_response_pump(handler.m_response_pump),
      m_channels(handler.m_channels)
{ }


std::string RequestHandler::GetRequestHttpVersion(const IHttpRequest* request)
{
    USHORT http_ver_major, http_ver_minor;
    request->GetHttpVersion(&http_ver_major, &http_ver_minor);
    std::string http_version = std::to_string(http_ver_major);
    if (http_ver_major == 1) {
        http_version += "." + std::to_string(http_ver_minor);
    }
    return http_version;
}

std::string RequestHandler::GetRequestScheme(const HTTP_REQUEST* raw_request)
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

std::string RequestHandler::GetRequestPath(const HTTP_REQUEST* raw_request)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(std::wstring(
        raw_request->CookedUrl.pAbsPath,
        raw_request->CookedUrl.AbsPathLength / sizeof(wchar_t)
    ));
}

std::string RequestHandler::GetRequestQueryString(const HTTP_REQUEST* raw_request)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(std::wstring(
        raw_request->CookedUrl.pQueryString,
        raw_request->CookedUrl.QueryStringLength / sizeof(wchar_t)
    ));
}

std::vector<std::tuple<std::string, std::string>> RequestHandler::GetRequestHeaders(const HTTP_REQUEST* raw_request)
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
