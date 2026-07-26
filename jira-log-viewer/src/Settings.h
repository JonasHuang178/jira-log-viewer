#pragma once

#include <windows.h>
#include <string>

struct PluginSettings {
    std::wstring focusedExtension;
    std::wstring focusedArchiveExt;
    std::wstring focusedPrefix;
    bool         focusedRecursive;
};

void InitSettings(HWND nppHandle);
void SaveSettings();
PluginSettings& GetSettings();
