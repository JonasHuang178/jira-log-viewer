#pragma once

#include <windows.h>
#include <string>
#include <vector>

class JiraClient;
struct JiraAttachment;

class FileHandler {
public:
    explicit FileHandler(const wchar_t* sevenZipPath);

    bool DownloadAttachment(JiraClient& client,
                            const JiraAttachment& att,
                            const std::wstring& jiraKey,
                            std::wstring& outLocalPath,
                            std::wstring& outError);

    bool Is7zFile(const std::wstring& path) const;

    bool Extract7z(const std::wstring& archivePath,
                   std::vector<std::wstring>& outFiles,
                   std::wstring& outError,
                   bool recursive = true);

private:
    std::wstring m_7zPath;

    std::wstring GetDownloadDir(const std::wstring& jiraKey);
    void EnsureDir(const std::wstring& path);
    void EnumFiles(const std::wstring& dir, std::vector<std::wstring>& out, bool recursive = true);
};
