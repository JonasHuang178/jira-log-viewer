#include "Settings.h"
#include "JiraConfig.h"
#include "PluginInterface.h"

static PluginSettings g_settings;
static std::wstring   g_iniPath;

static std::wstring ReadIniStr(const wchar_t* key, const wchar_t* def)
{
    wchar_t buf[512] = {};
    ::GetPrivateProfileStringW(L"Settings", key, def,
                               buf, _countof(buf), g_iniPath.c_str());
    return buf;
}

static void WriteIniStr(const wchar_t* key, const std::wstring& val)
{
    ::WritePrivateProfileStringW(L"Settings", key, val.c_str(), g_iniPath.c_str());
}

void InitSettings(HWND nppHandle)
{
    wchar_t configDir[MAX_PATH] = {};
    ::SendMessageW(nppHandle, NPPM_GETPLUGINSCONFIGDIR,
                   MAX_PATH, reinterpret_cast<LPARAM>(configDir));

    g_iniPath = configDir;
    g_iniPath += L"\\jira-log-viewer.ini";

    g_settings.focusedExtension  = ReadIniStr(L"FocusedExtension",  FOCUSED_EXTENSION);
    g_settings.focusedArchiveExt = ReadIniStr(L"FocusedArchiveExt", FOCUSED_ARCHIVE_EXT);
    g_settings.focusedPrefix     = ReadIniStr(L"FocusedPrefix",     FOCUSED_PREFIX);
    g_settings.focusedRecursive  = ::GetPrivateProfileIntW(
        L"Settings", L"FocusedRecursive", FOCUSED_RECURSIVE ? 1 : 0,
        g_iniPath.c_str()) != 0;
}

void SaveSettings()
{
    WriteIniStr(L"FocusedExtension",  g_settings.focusedExtension);
    WriteIniStr(L"FocusedArchiveExt", g_settings.focusedArchiveExt);
    WriteIniStr(L"FocusedPrefix",     g_settings.focusedPrefix);
    WriteIniStr(L"FocusedRecursive",  g_settings.focusedRecursive ? L"1" : L"0");
}

PluginSettings& GetSettings()
{
    return g_settings;
}
