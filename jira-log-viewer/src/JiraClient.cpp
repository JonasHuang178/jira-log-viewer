#include "JiraClient.h"
#include <winhttp.h>
#include <shlwapi.h>
#include <nlohmann/json.hpp>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::wstring Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) return {};
    int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(len, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                          static_cast<int>(utf8.size()), wide.data(), len);
    return wide;
}

static std::string WideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return {};
    int len = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                    static_cast<int>(wide.size()),
                                    nullptr, 0, nullptr, nullptr);
    std::string utf8(len, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                          static_cast<int>(wide.size()),
                          utf8.data(), len, nullptr, nullptr);
    return utf8;
}

static std::wstring FormatWinHttpError(DWORD err)
{
    wchar_t buf[256];
    ::swprintf_s(buf, L"WinHTTP error: %lu", err);
    return buf;
}

struct UrlParts {
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
    bool isHttps = true;
};

static UrlParts ParseUrl(const std::wstring& url)
{
    UrlParts parts;
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);

    wchar_t hostBuf[256] = {};
    wchar_t pathBuf[2048] = {};
    uc.lpszHostName = hostBuf;
    uc.dwHostNameLength = _countof(hostBuf);
    uc.lpszUrlPath = pathBuf;
    uc.dwUrlPathLength = _countof(pathBuf);

    if (::WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) {
        parts.host = hostBuf;
        parts.path = pathBuf;
        parts.port = uc.nPort;
        parts.isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    }
    return parts;
}

// ---------------------------------------------------------------------------
// JiraClient
// ---------------------------------------------------------------------------
JiraClient::JiraClient(const wchar_t* baseUrl, const wchar_t* pat)
    : m_baseUrl(baseUrl)
    , m_pat(pat)
{
    while (!m_baseUrl.empty() && m_baseUrl.back() == L'/')
        m_baseUrl.pop_back();
}

std::string JiraClient::HttpGet(const std::wstring& url,
                                 DWORD& outStatusCode,
                                 std::wstring& outError)
{
    outStatusCode = 0;
    auto parts = ParseUrl(url);
    if (parts.host.empty()) {
        outError = L"Failed to parse URL: " + url;
        return {};
    }

    HINTERNET hSession = ::WinHttpOpen(L"jira-log-viewer/1.0",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        outError = FormatWinHttpError(::GetLastError());
        return {};
    }

    DWORD connectTimeout = 30000;
    DWORD receiveTimeout = 60000;
    ::WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT,
                       &connectTimeout, sizeof(connectTimeout));
    ::WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT,
                       &receiveTimeout, sizeof(receiveTimeout));

    HINTERNET hConnect = ::WinHttpConnect(hSession, parts.host.c_str(),
                                          parts.port, 0);
    if (!hConnect) {
        outError = FormatWinHttpError(::GetLastError());
        ::WinHttpCloseHandle(hSession);
        return {};
    }

    DWORD flags = parts.isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = ::WinHttpOpenRequest(hConnect, L"GET",
                                               parts.path.c_str(),
                                               nullptr, WINHTTP_NO_REFERER,
                                               WINHTTP_DEFAULT_ACCEPT_TYPES,
                                               flags);
    if (!hRequest) {
        outError = FormatWinHttpError(::GetLastError());
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return {};
    }

    if (parts.isHttps) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        ::WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS,
                           &secFlags, sizeof(secFlags));
    }

    std::wstring authHeader = L"Authorization: Bearer " + m_pat;
    ::WinHttpAddRequestHeaders(hRequest, authHeader.c_str(),
                                static_cast<DWORD>(authHeader.size()),
                                WINHTTP_ADDREQ_FLAG_ADD);

    DWORD disableRedirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    ::WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY,
                       &disableRedirect, sizeof(disableRedirect));

    if (!::WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        outError = FormatWinHttpError(::GetLastError());
        ::WinHttpCloseHandle(hRequest);
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return {};
    }

    if (!::WinHttpReceiveResponse(hRequest, nullptr)) {
        outError = FormatWinHttpError(::GetLastError());
        ::WinHttpCloseHandle(hRequest);
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return {};
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    ::WinHttpQueryHeaders(hRequest,
                          WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                          WINHTTP_HEADER_NAME_BY_INDEX,
                          &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    outStatusCode = statusCode;

    if (statusCode == 302) {
        wchar_t location[2048] = {};
        DWORD locSize = sizeof(location);
        ::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION,
                              WINHTTP_HEADER_NAME_BY_INDEX,
                              location, &locSize, WINHTTP_NO_HEADER_INDEX);
        std::wstring loc(location);
        if (loc.find(L"/login") != std::wstring::npos) {
            outError = L"Jira redirected to login page. Your Jira Server version "
                       L"may not support PAT for attachment downloads "
                       L"(JRASERVER-72019). Please check your Jira version.";
        } else {
            outError = L"Unexpected redirect to: " + loc;
        }
        ::WinHttpCloseHandle(hRequest);
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return {};
    }

    std::string body;
    DWORD bytesAvailable = 0;
    while (::WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buf(bytesAvailable);
        DWORD bytesRead = 0;
        if (::WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead))
            body.append(buf.data(), bytesRead);
    }

    ::WinHttpCloseHandle(hRequest);
    ::WinHttpCloseHandle(hConnect);
    ::WinHttpCloseHandle(hSession);
    return body;
}

bool JiraClient::HttpGetToFile(const std::wstring& url,
                                const std::wstring& localPath,
                                DWORD& outStatusCode,
                                std::wstring& outError)
{
    outStatusCode = 0;
    auto parts = ParseUrl(url);
    if (parts.host.empty()) {
        outError = L"Failed to parse URL: " + url;
        return false;
    }

    HINTERNET hSession = ::WinHttpOpen(L"jira-log-viewer/1.0",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { outError = FormatWinHttpError(::GetLastError()); return false; }

    DWORD connectTimeout = 30000;
    DWORD receiveTimeout = 60000;
    ::WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT,
                       &connectTimeout, sizeof(connectTimeout));
    ::WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT,
                       &receiveTimeout, sizeof(receiveTimeout));

    HINTERNET hConnect = ::WinHttpConnect(hSession, parts.host.c_str(),
                                          parts.port, 0);
    if (!hConnect) {
        outError = FormatWinHttpError(::GetLastError());
        ::WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = parts.isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = ::WinHttpOpenRequest(hConnect, L"GET",
                                               parts.path.c_str(),
                                               nullptr, WINHTTP_NO_REFERER,
                                               WINHTTP_DEFAULT_ACCEPT_TYPES,
                                               flags);
    if (!hRequest) {
        outError = FormatWinHttpError(::GetLastError());
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return false;
    }

    if (parts.isHttps) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        ::WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS,
                           &secFlags, sizeof(secFlags));
    }

    std::wstring authHeader = L"Authorization: Bearer " + m_pat;
    ::WinHttpAddRequestHeaders(hRequest, authHeader.c_str(),
                                static_cast<DWORD>(authHeader.size()),
                                WINHTTP_ADDREQ_FLAG_ADD);

    DWORD disableRedirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    ::WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY,
                       &disableRedirect, sizeof(disableRedirect));

    if (!::WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !::WinHttpReceiveResponse(hRequest, nullptr)) {
        outError = FormatWinHttpError(::GetLastError());
        ::WinHttpCloseHandle(hRequest);
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    ::WinHttpQueryHeaders(hRequest,
                          WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                          WINHTTP_HEADER_NAME_BY_INDEX,
                          &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    outStatusCode = statusCode;

    if (statusCode == 302) {
        wchar_t location[2048] = {};
        DWORD locSize = sizeof(location);
        ::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION,
                              WINHTTP_HEADER_NAME_BY_INDEX,
                              location, &locSize, WINHTTP_NO_HEADER_INDEX);
        std::wstring loc(location);
        if (loc.find(L"/login") != std::wstring::npos)
            outError = L"Jira redirected to login page (JRASERVER-72019).";
        else
            outError = L"Unexpected redirect to: " + loc;
        ::WinHttpCloseHandle(hRequest);
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return false;
    }

    if (statusCode != 200) {
        wchar_t buf[128];
        ::swprintf_s(buf, L"HTTP %lu", statusCode);
        outError = buf;
        ::WinHttpCloseHandle(hRequest);
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return false;
    }

    HANDLE hFile = ::CreateFileW(localPath.c_str(), GENERIC_WRITE, 0, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        outError = L"Failed to create file: " + localPath;
        ::WinHttpCloseHandle(hRequest);
        ::WinHttpCloseHandle(hConnect);
        ::WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD bytesAvailable = 0;
    bool ok = true;
    while (::WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buf(bytesAvailable);
        DWORD bytesRead = 0;
        if (::WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead)) {
            DWORD written = 0;
            if (!::WriteFile(hFile, buf.data(), bytesRead, &written, nullptr)) {
                outError = L"Failed to write file";
                ok = false;
                break;
            }
        }
    }

    ::CloseHandle(hFile);
    ::WinHttpCloseHandle(hRequest);
    ::WinHttpCloseHandle(hConnect);
    ::WinHttpCloseHandle(hSession);
    return ok;
}

std::vector<JiraAttachment> JiraClient::GetAttachments(
    const std::wstring& issueKey,
    std::wstring& outTitle,
    std::wstring& outError)
{
    std::wstring url = m_baseUrl + L"/rest/api/2/issue/" + issueKey;

    DWORD statusCode = 0;
    std::string body = HttpGet(url, statusCode, outError);

    if (!outError.empty())
        return {};

    if (statusCode == 401 || statusCode == 403) {
        outError = L"Authentication failed (HTTP " + std::to_wstring(statusCode) +
                   L"). Please check the PAT in config/JiraConfig.h.";
        return {};
    }
    if (statusCode == 404) {
        outError = L"Issue not found: " + issueKey;
        return {};
    }
    if (statusCode != 200) {
        outError = L"Jira API returned HTTP " + std::to_wstring(statusCode);
        return {};
    }

    std::vector<JiraAttachment> result;
    try {
        auto j = json::parse(body);

        if (j.contains("fields") && j["fields"].contains("summary"))
            outTitle = Utf8ToWide(j["fields"]["summary"].get<std::string>());

        if (j.contains("fields") && j["fields"].contains("attachment")) {
            for (const auto& att : j["fields"]["attachment"]) {
                JiraAttachment a;
                if (att.contains("id"))
                    a.id = Utf8ToWide(att["id"].get<std::string>());
                if (att.contains("filename"))
                    a.filename = Utf8ToWide(att["filename"].get<std::string>());
                if (att.contains("size"))
                    a.size = att["size"].get<int64_t>();
                if (att.contains("created"))
                    a.created = Utf8ToWide(att["created"].get<std::string>());
                if (att.contains("author") && att["author"].contains("displayName"))
                    a.author = Utf8ToWide(att["author"]["displayName"].get<std::string>());
                if (att.contains("content"))
                    a.contentUrl = Utf8ToWide(att["content"].get<std::string>());
                result.push_back(std::move(a));
            }
        }
    }
    catch (const json::exception& e) {
        outError = L"JSON parse error: " + Utf8ToWide(e.what());
        return {};
    }

    return result;
}

bool JiraClient::DownloadFile(const std::wstring& url,
                               const std::wstring& localPath,
                               std::wstring& outError)
{
    DWORD statusCode = 0;
    return HttpGetToFile(url, localPath, statusCode, outError);
}
