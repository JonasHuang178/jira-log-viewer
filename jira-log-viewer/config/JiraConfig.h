#pragma once

// -----------------------------------------------------------------------------
//  JiraConfig.h - Jira connection settings (compile-time)
//  Edit this file and rebuild to change Jira server settings.
// -----------------------------------------------------------------------------

// Jira Server base URL (no trailing slash)
constexpr wchar_t JIRA_URL[] = L"https://jira.yourcompany.com";

// Personal Access Token for Jira Server/DC
constexpr wchar_t JIRA_PAT[] = L"your-personal-access-token";

// Path to 7-Zip command-line tool (for extracting .7z attachments)
constexpr wchar_t SEVEN_ZIP_PATH[] = L"C:\\Program Files\\7-Zip\\7z.exe";

// Focused file extension filter (e.g. L".log", L".txt")
// Attachments matching this extension are prioritized:
//   - Single match: open directly without dialog
//   - Multiple matches: show selection dialog with only matching files
//   - Empty string: show all attachments (no filter)
constexpr wchar_t FOCUSED_EXTENSION[] = L".log";

// Focused archive settings
// Archive extension to look for (e.g. L".7z", L".zip")
constexpr wchar_t FOCUSED_ARCHIVE_EXT[] = L".7z";
// Only consider archives whose filename starts with this prefix
// Empty string means no prefix filter (all archives with the extension match)
constexpr wchar_t FOCUSED_PREFIX[] = L"Pattern_";
// Search extracted contents recursively (true) or top-level only (false)
constexpr bool FOCUSED_RECURSIVE = false;

// Shortcut key for "Open Jira Attachment"
// Modifier keys: set to true to enable
constexpr bool SHORTCUT_CTRL  = true;
constexpr bool SHORTCUT_ALT   = false;
constexpr bool SHORTCUT_SHIFT = true;
// Main key: virtual key code (e.g. 'J', 'K', VK_F5)
// See: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
constexpr UCHAR SHORTCUT_KEY  = 'J';
