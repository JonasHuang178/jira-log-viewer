#include "FileHandler.h"
#include "JiraClient.h"
#include <shlwapi.h>

FileHandler::FileHandler(const wchar_t* sevenZipPath)
    : m_7zPath(sevenZipPath)
{
}

std::wstring FileHandler::GetDownloadDir(const std::wstring& jiraKey)
{
    wchar_t tempPath[MAX_PATH] = {};
    ::GetTempPathW(MAX_PATH, tempPath);

    std::wstring dir = tempPath;
    dir += L"jira-log-viewer\\";
    dir += jiraKey;
    dir += L"\\";
    return dir;
}

void FileHandler::EnsureDir(const std::wstring& path)
{
    // Create all directories in path
    wchar_t buf[MAX_PATH];
    wcscpy_s(buf, path.c_str());

    for (wchar_t* p = buf; *p; ++p) {
        if (*p == L'\\' || *p == L'/') {
            wchar_t saved = *p;
            *p = L'\0';
            ::CreateDirectoryW(buf, nullptr);
            *p = saved;
        }
    }
    ::CreateDirectoryW(buf, nullptr);
}

void FileHandler::EnumFiles(const std::wstring& dir, std::vector<std::wstring>& out, bool recursive)
{
    std::wstring pattern = dir + L"*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = ::FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive && wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) {
                std::wstring subDir = dir + fd.cFileName + L"\\";
                EnumFiles(subDir, out, true);
            }
        } else {
            out.push_back(dir + fd.cFileName);
        }
    } while (::FindNextFileW(hFind, &fd));

    ::FindClose(hFind);
}

bool FileHandler::DownloadAttachment(JiraClient& client,
                                      const JiraAttachment& att,
                                      const std::wstring& jiraKey,
                                      std::wstring& outLocalPath,
                                      std::wstring& outError)
{
    std::wstring dir = GetDownloadDir(jiraKey);
    EnsureDir(dir);

    outLocalPath = dir + att.filename;
    return client.DownloadFile(att.contentUrl, outLocalPath, outError);
}

bool FileHandler::Is7zFile(const std::wstring& path) const
{
    const wchar_t* ext = ::PathFindExtensionW(path.c_str());
    return ext && _wcsicmp(ext, L".7z") == 0;
}

bool FileHandler::Extract7z(const std::wstring& archivePath,
                              std::vector<std::wstring>& outFiles,
                              std::wstring& outError,
                              bool recursive)
{
    if (!::PathFileExistsW(m_7zPath.c_str())) {
        outError = L"7z.exe not found at: " + m_7zPath +
                   L"\nPlease check SEVEN_ZIP_PATH in config/JiraConfig.h";
        return false;
    }

    // Output dir = archive path without extension
    std::wstring outputDir = archivePath;
    size_t dotPos = outputDir.rfind(L'.');
    if (dotPos != std::wstring::npos)
        outputDir = outputDir.substr(0, dotPos);
    outputDir += L"\\";
    EnsureDir(outputDir);

    // Build command: 7z.exe x -o"outputDir" -y "archivePath"
    std::wstring cmdLine = L"\"" + m_7zPath + L"\" x -o\"" + outputDir + L"\" -y \"" + archivePath + L"\"";

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hReadErr = nullptr, hWriteErr = nullptr;
    ::CreatePipe(&hReadErr, &hWriteErr, &sa, 0);
    ::SetHandleInformation(hReadErr, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdError = hWriteErr;
    si.hStdOutput = hWriteErr;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    BOOL created = ::CreateProcessW(
        nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    ::CloseHandle(hWriteErr);

    if (!created) {
        ::CloseHandle(hReadErr);
        outError = L"Failed to launch 7z.exe";
        return false;
    }

    // Read stderr/stdout
    std::string output;
    char buf[4096];
    DWORD bytesRead;
    while (::ReadFile(hReadErr, buf, sizeof(buf), &bytesRead, nullptr) && bytesRead > 0)
        output.append(buf, bytesRead);
    ::CloseHandle(hReadErr);

    ::WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    ::GetExitCodeProcess(pi.hProcess, &exitCode);
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);

    if (exitCode != 0) {
        int len = ::MultiByteToWideChar(CP_UTF8, 0, output.data(),
                                        static_cast<int>(output.size()), nullptr, 0);
        std::wstring wOutput(len, L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, output.data(),
                              static_cast<int>(output.size()), wOutput.data(), len);
        outError = L"7z extraction failed (exit code " + std::to_wstring(exitCode) +
                   L"):\n" + wOutput;
        return false;
    }

    EnumFiles(outputDir, outFiles, recursive);
    return true;
}
