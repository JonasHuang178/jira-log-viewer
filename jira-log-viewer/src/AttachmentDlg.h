#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "JiraClient.h"

// Show attachment selection dialog. Returns selected attachments.
std::vector<JiraAttachment> ShowAttachmentDlg(
    HINSTANCE hInst,
    HWND hParent,
    const std::wstring& jiraKey,
    const std::wstring& issueTitle,
    const std::vector<JiraAttachment>& attachments);

// Show file selection dialog after .7z extraction. Returns selected file paths.
std::vector<std::wstring> ShowFileDlg(
    HINSTANCE hInst,
    HWND hParent,
    const std::wstring& jiraKey,
    const std::wstring& archiveName,
    const std::vector<std::wstring>& files);
