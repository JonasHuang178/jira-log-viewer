#pragma once

// -----------------------------------------------------------------------------
//  AboutInfo.h - Plugin metadata displayed in the About dialog
//  Edit this file to update the version or description shown to users.
// -----------------------------------------------------------------------------

#define PLUGIN_VERSION   L"v1.0"
#define PLUGIN_NAME      L"jira-log-viewer"

#define ABOUT_TITLE \
    PLUGIN_NAME L" " PLUGIN_VERSION

#define ABOUT_CONTENT \
    L"Plugin : jira-log-viewer " PLUGIN_VERSION     L"\n" \
    L"Shortcut: Ctrl+Shift+J"                       L"\n" \
    L""                                              L"\n" \
    L"Download and open Jira attachments directly"   L"\n" \
    L"in Notepad++."                                 L"\n"
