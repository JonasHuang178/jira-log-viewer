# jira-log-viewer

Notepad++ plugin for downloading and opening Jira ticket attachments directly in the editor.

Supports Jira Server/DC with PAT authentication, HTTPS self-signed certificates, and .7z archive extraction.

## Features

- Input a Jira Key to fetch the ticket's attachment list via REST API
- Smart attachment filtering with two-path priority:
  - **Archive path**: auto-find archives matching prefix + extension (e.g. `Pattern_*.7z`), extract, then filter for target files (e.g. `.log`)
  - **Direct path**: fallback to direct file extension matching
- Single match auto-opens without dialog; multiple matches show selection
- Browse attachments in a ListView dialog (No | Date | Filename) with checkboxes, text filter, Select All, and double-click to open
- Download attachments to `%TEMP%\jira-log-viewer\{JIRA-KEY}\`
- Automatic .7z extraction with configurable recursive/top-level search
- Runtime Settings dialog for filter configuration (saved to INI, takes effect immediately)
- Configurable keyboard shortcut (default: `Ctrl+Shift+J`)

## Requirements

- Windows 10+ (64-bit)
- Notepad++ 64-bit
- Visual Studio 2022 (v143 toolset) for building
- [7-Zip](https://www.7-zip.org/) installed (for .7z extraction)
- Jira Server/DC instance with PAT enabled

## Configuration

### Compile-time settings (config/JiraConfig.h)

Edit before building. These require recompilation to change.

```cpp
// Jira Server base URL (no trailing slash)
constexpr wchar_t JIRA_URL[] = L"https://jira.yourcompany.com";

// Personal Access Token for Jira Server/DC
constexpr wchar_t JIRA_PAT[] = L"your-personal-access-token";

// Path to 7-Zip command-line tool
constexpr wchar_t SEVEN_ZIP_PATH[] = L"C:\\Program Files\\7-Zip\\7z.exe";

// Shortcut key for "Open Jira Attachment"
constexpr bool  SHORTCUT_CTRL  = true;
constexpr bool  SHORTCUT_ALT   = false;
constexpr bool  SHORTCUT_SHIFT = true;
constexpr UCHAR SHORTCUT_KEY   = 'J';  // virtual key code
```

### Runtime settings (Settings dialog)

Accessible from **Plugins** > **jira-log-viewer** > **Settings**. Changes take effect immediately and persist in `jira-log-viewer.ini` in the Notepad++ plugin config directory.

| Setting | Default | Description |
|---------|---------|-------------|
| Focused Extension | `.log` | File extension to prioritize when filtering attachments or extracted files |
| Archive Extension | `.7z` | Archive extension to look for in the focused archive path |
| Archive Prefix | `Pattern_` | Only archives whose filename starts with this prefix are considered (empty = no filter) |
| Search recursively | Off | Whether to search subdirectories after extracting an archive |

The compile-time defaults in `JiraConfig.h` are used on first launch when no INI file exists.

### config/AboutInfo.h

Plugin name, version, and About dialog content.

## Build

1. Open `jira-log-viewer.sln` in Visual Studio 2022
2. Edit `config/JiraConfig.h` with your Jira URL and PAT
3. Select **Release | x64** configuration
4. Build Solution (`Ctrl+Shift+B`)
5. Output: `x64\Release\jira-log-viewer.dll`

## Install

1. In Notepad++ plugins directory (e.g. `C:\Program Files\Notepad++\plugins\`), create a folder named `jira-log-viewer`
2. Copy `jira-log-viewer.dll` into that folder
3. Restart Notepad++
4. The plugin appears under **Plugins** > **jira-log-viewer**

## Usage

1. Press `Ctrl+Shift+J` (or your configured shortcut)
2. Enter a Jira Key (e.g. `PROJ-123`) in the input dialog
3. The plugin automatically filters attachments:
   - If a matching archive (e.g. `Pattern_*.7z`) is found, it downloads and extracts it, then opens `.log` files inside
   - If only direct `.log` files exist, they are opened directly
   - Multiple matches show a selection dialog
4. Double-click any row to open that file immediately
5. Files open as new tabs in Notepad++

## Project Structure

```
jira-log-viewer/
├── config/
│   ├── AboutInfo.h            # Plugin name/version/About content
│   └── JiraConfig.h           # Jira URL, PAT, 7z path, shortcut, filter defaults
├── external/
│   ├── PluginInterface.h      # Notepad++ plugin SDK
│   ├── Scintilla.h            # Scintilla editor interface
│   └── nlohmann/
│       └── json.hpp           # nlohmann/json v3.11.3 (header-only)
├── src/
│   ├── dllmain.cpp            # DLL entry point
│   ├── Plugin.h / .cpp        # 6 DLL exports + command functions
│   ├── JiraClient.h / .cpp    # WinHTTP REST API client
│   ├── FileHandler.h / .cpp   # Download, extract, file enumeration
│   ├── InputDlg.h / .cpp      # Jira Key input dialog
│   ├── AttachmentDlg.h / .cpp # Attachment/file selection dialog
│   ├── Settings.h / .cpp      # INI settings load/save + runtime state
│   └── SettingsDlg.h / .cpp   # Settings dialog UI
├── jira-log-viewer.sln
├── jira-log-viewer.vcxproj
├── jira-log-viewer.vcxproj.filters
└── jira-log-viewer.def
```

## Technical Details

- **Build**: VS 2022, v143 toolset, C++17, Unicode, x64
- **HTTP**: WinHTTP (Windows built-in), self-signed cert bypass via `WINHTTP_OPTION_SECURITY_FLAGS`
- **Auth**: Bearer PAT, redirect detection for JRASERVER-72019 compatibility
- **JSON**: nlohmann/json (header-only, `external/nlohmann/json.hpp`)
- **Archive**: `CreateProcess` calling `7z.exe` with stderr pipe capture, configurable recursive enumeration
- **Dialogs**: Programmatic Win32 (`CreateWindowExW` + custom WndProc), no .rc resources
- **Settings**: INI via `WritePrivateProfileString`/`GetPrivateProfileString`, config dir via `NPPM_GETPLUGINSCONFIGDIR`
- **Timeouts**: 30s connect, 60s receive
- **Linked libraries**: `winhttp.lib`, `comctl32.lib`, `shlwapi.lib`
