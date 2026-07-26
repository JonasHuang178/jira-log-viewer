#include "Plugin.h"
#include "JiraClient.h"
#include "FileHandler.h"
#include "InputDlg.h"
#include "AttachmentDlg.h"
#include "AboutInfo.h"
#include "JiraConfig.h"
#include "Settings.h"
#include "SettingsDlg.h"
#include "Scintilla.h"
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
NppData g_nppData = {};

static FuncItem    g_funcItems[3];
static ShortcutKey g_openJiraKey;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
HWND GetCurrentScintilla()
{
    int currentView = 0;
    ::SendMessage(g_nppData._nppHandle,
                  NPPM_GETCURRENTSCINTILLA,
                  0,
                  reinterpret_cast<LPARAM>(&currentView));
    return (currentView == 0) ? g_nppData._scintillaMainHandle
                               : g_nppData._scintillaSecondHandle;
}

// ---------------------------------------------------------------------------
// Command: Open Jira Attachment  (Ctrl+Shift+J)
// ---------------------------------------------------------------------------
static void OpenJiraAttachment()
{
    std::wstring jiraKey = ShowInputDlg(g_hInstance, g_nppData._nppHandle);
    if (jiraKey.empty())
        return;

    HCURSOR oldCursor = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

    JiraClient client(JIRA_URL, JIRA_PAT);
    std::wstring issueTitle;
    std::wstring error;
    auto attachments = client.GetAttachments(jiraKey, issueTitle, error);

    ::SetCursor(oldCursor);

    if (!error.empty()) {
        ::MessageBoxW(g_nppData._nppHandle, error.c_str(),
                      L"jira-log-viewer", MB_OK | MB_ICONERROR);
        return;
    }

    if (attachments.empty()) {
        ::MessageBoxW(g_nppData._nppHandle,
                      L"No attachments found for this issue.",
                      L"jira-log-viewer", MB_OK | MB_ICONINFORMATION);
        return;
    }

    const auto& cfg = GetSettings();
    FileHandler handler(SEVEN_ZIP_PATH);

    // --- Path A: focused archive (prefix + archive extension) ---
    std::vector<JiraAttachment> focusedArchives;
    if (!cfg.focusedArchiveExt.empty()) {
        for (const auto& att : attachments) {
            const wchar_t* ext = ::PathFindExtensionW(att.filename.c_str());
            if (!ext || _wcsicmp(ext, cfg.focusedArchiveExt.c_str()) != 0)
                continue;
            if (!cfg.focusedPrefix.empty() &&
                _wcsnicmp(att.filename.c_str(), cfg.focusedPrefix.c_str(), cfg.focusedPrefix.size()) != 0)
                continue;
            focusedArchives.push_back(att);
        }
    }

    if (!focusedArchives.empty()) {
        std::vector<JiraAttachment> selectedArchives;
        if (focusedArchives.size() == 1) {
            selectedArchives.push_back(focusedArchives[0]);
        } else {
            selectedArchives = ShowAttachmentDlg(g_hInstance, g_nppData._nppHandle,
                                                 jiraKey, issueTitle, focusedArchives);
        }
        if (selectedArchives.empty())
            return;

        oldCursor = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

        for (const auto& att : selectedArchives)
        {
            std::wstring localPath;
            std::wstring dlError;
            if (!handler.DownloadAttachment(client, att, jiraKey, localPath, dlError)) {
                ::SetCursor(oldCursor);
                ::MessageBoxW(g_nppData._nppHandle, dlError.c_str(),
                              L"Download Error", MB_OK | MB_ICONERROR);
                continue;
            }

            std::vector<std::wstring> extracted;
            std::wstring extractError;
            if (!handler.Extract7z(localPath, extracted, extractError, cfg.focusedRecursive)) {
                ::SetCursor(oldCursor);
                ::MessageBoxW(g_nppData._nppHandle, extractError.c_str(),
                              L"Extract Error", MB_OK | MB_ICONERROR);
                continue;
            }

            std::vector<std::wstring> matched;
            if (!cfg.focusedExtension.empty()) {
                for (const auto& f : extracted) {
                    const wchar_t* ext = ::PathFindExtensionW(f.c_str());
                    if (ext && _wcsicmp(ext, cfg.focusedExtension.c_str()) == 0)
                        matched.push_back(f);
                }
            }
            if (matched.empty())
                matched = extracted;

            ::SetCursor(oldCursor);

            if (matched.size() == 1) {
                ::SendMessage(g_nppData._nppHandle, NPPM_DOOPEN, 0,
                              reinterpret_cast<LPARAM>(matched[0].c_str()));
            } else {
                auto selectedFiles = ShowFileDlg(g_hInstance, g_nppData._nppHandle,
                                                  jiraKey, att.filename, matched);
                for (const auto& f : selectedFiles) {
                    ::SendMessage(g_nppData._nppHandle, NPPM_DOOPEN, 0,
                                  reinterpret_cast<LPARAM>(f.c_str()));
                }
            }

            oldCursor = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
        }
        ::SetCursor(oldCursor);
        return;
    }

    // --- Path B: direct file filtering by focused extension ---
    std::vector<JiraAttachment> focused;
    if (!cfg.focusedExtension.empty()) {
        for (const auto& att : attachments) {
            const wchar_t* ext = ::PathFindExtensionW(att.filename.c_str());
            if (ext && _wcsicmp(ext, cfg.focusedExtension.c_str()) == 0)
                focused.push_back(att);
        }
    }

    std::vector<JiraAttachment> selected;
    const auto& candidates = (focused.empty()) ? attachments : focused;

    if (candidates.size() == 1) {
        selected.push_back(candidates[0]);
    } else {
        selected = ShowAttachmentDlg(g_hInstance, g_nppData._nppHandle,
                                     jiraKey, issueTitle, candidates);
    }

    if (selected.empty())
        return;

    oldCursor = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

    for (const auto& att : selected)
    {
        std::wstring localPath;
        std::wstring dlError;

        if (!handler.DownloadAttachment(client, att, jiraKey, localPath, dlError)) {
            ::SetCursor(oldCursor);
            ::MessageBoxW(g_nppData._nppHandle, dlError.c_str(),
                          L"Download Error", MB_OK | MB_ICONERROR);
            continue;
        }

        if (handler.Is7zFile(localPath))
        {
            std::vector<std::wstring> extracted;
            std::wstring extractError;

            if (!handler.Extract7z(localPath, extracted, extractError)) {
                ::SetCursor(oldCursor);
                ::MessageBoxW(g_nppData._nppHandle, extractError.c_str(),
                              L"Extract Error", MB_OK | MB_ICONERROR);
                continue;
            }

            ::SetCursor(oldCursor);

            if (extracted.size() == 1) {
                ::SendMessage(g_nppData._nppHandle, NPPM_DOOPEN, 0,
                              reinterpret_cast<LPARAM>(extracted[0].c_str()));
            }
            else if (!extracted.empty()) {
                auto selectedFiles = ShowFileDlg(g_hInstance, g_nppData._nppHandle,
                                                 jiraKey, att.filename, extracted);
                for (const auto& f : selectedFiles) {
                    ::SendMessage(g_nppData._nppHandle, NPPM_DOOPEN, 0,
                                  reinterpret_cast<LPARAM>(f.c_str()));
                }
            }

            oldCursor = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
        }
        else
        {
            ::SendMessage(g_nppData._nppHandle, NPPM_DOOPEN, 0,
                          reinterpret_cast<LPARAM>(localPath.c_str()));
        }
    }

    ::SetCursor(oldCursor);
}

// ---------------------------------------------------------------------------
// Command: Settings
// ---------------------------------------------------------------------------
static void OpenSettings()
{
    ShowSettingsDlg(g_hInstance, g_nppData._nppHandle);
}

// ---------------------------------------------------------------------------
// Command: About
// ---------------------------------------------------------------------------
static void ShowAbout()
{
    ::MessageBoxW(g_nppData._nppHandle,
                  ABOUT_CONTENT,
                  ABOUT_TITLE,
                  MB_OK | MB_ICONINFORMATION);
}

// ---------------------------------------------------------------------------
// Notepad++ Plugin API exports
// ---------------------------------------------------------------------------
extern "C" {

__declspec(dllexport) bool isUnicode()
{
    return true;
}

__declspec(dllexport) const TCHAR* getName()
{
    return TEXT("jira-log-viewer");
}

__declspec(dllexport) FuncItem* getFuncsArray(int* nbF)
{
    *nbF = 3;

    _tcscpy_s(g_funcItems[0]._itemName, TEXT("Open Jira Attachment"));
    g_funcItems[0]._pFunc      = OpenJiraAttachment;
    g_funcItems[0]._cmdID      = 0;
    g_funcItems[0]._init2Check = false;

    g_openJiraKey._isCtrl  = SHORTCUT_CTRL;
    g_openJiraKey._isAlt   = SHORTCUT_ALT;
    g_openJiraKey._isShift = SHORTCUT_SHIFT;
    g_openJiraKey._key     = SHORTCUT_KEY;
    g_funcItems[0]._pShKey = &g_openJiraKey;

    _tcscpy_s(g_funcItems[1]._itemName, TEXT("Settings"));
    g_funcItems[1]._pFunc      = OpenSettings;
    g_funcItems[1]._cmdID      = 0;
    g_funcItems[1]._init2Check = false;
    g_funcItems[1]._pShKey     = nullptr;

    _tcscpy_s(g_funcItems[2]._itemName, TEXT("About"));
    g_funcItems[2]._pFunc      = ShowAbout;
    g_funcItems[2]._cmdID      = 0;
    g_funcItems[2]._init2Check = false;
    g_funcItems[2]._pShKey     = nullptr;

    return g_funcItems;
}

__declspec(dllexport) void setInfo(NppData notepadPlusData)
{
    g_nppData = notepadPlusData;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES };
    ::InitCommonControlsEx(&icc);

    InitSettings(g_nppData._nppHandle);
}

__declspec(dllexport) void beNotified(SCNotification* notification)
{
    if (!notification) return;
    // No notifications to handle for this plugin
}

__declspec(dllexport) LRESULT messageProc(UINT    /*Message*/,
                                           WPARAM  /*wParam*/,
                                           LPARAM  /*lParam*/)
{
    return TRUE;
}

} // extern "C"
