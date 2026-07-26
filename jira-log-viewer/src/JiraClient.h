#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct JiraAttachment {
    std::wstring id;
    std::wstring filename;
    int64_t      size = 0;
    std::wstring created;
    std::wstring author;
    std::wstring contentUrl;
};

class JiraClient {
public:
    JiraClient(const wchar_t* baseUrl, const wchar_t* pat);

    std::vector<JiraAttachment> GetAttachments(
        const std::wstring& issueKey,
        std::wstring& outTitle,
        std::wstring& outError);

    bool DownloadFile(const std::wstring& url,
                      const std::wstring& localPath,
                      std::wstring& outError);

private:
    std::wstring m_baseUrl;
    std::wstring m_pat;

    std::string HttpGet(const std::wstring& url,
                        DWORD& outStatusCode,
                        std::wstring& outError);

    bool HttpGetToFile(const std::wstring& url,
                       const std::wstring& localPath,
                       DWORD& outStatusCode,
                       std::wstring& outError);
};
