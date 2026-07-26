# jira-log-viewer

Notepad++ plugin that downloads and opens Jira ticket log attachments directly in the editor, eliminating the manual workflow of browsing Jira, downloading files, and opening them one by one.

Designed for corporate Jira Server/DC environments with HTTPS self-signed certificates and Personal Access Token (PAT) authentication.

## Features

### Smart Attachment Filtering

The plugin uses a two-path priority system to minimize clicks:

**Path A (Archive priority)** — If the Jira ticket has `.7z` archives matching a configurable prefix (default `Pattern_`):
1. Single match: auto-downloads and extracts without prompting
2. Multiple matches: shows a selection dialog
3. After extraction, filters for target files (default `.log`)
4. Single `.log` found: opens directly; multiple: shows selection dialog

**Path B (Direct file fallback)** — If no matching archives exist:
1. Filters attachments by focused extension (default `.log`)
2. Single match: opens directly; multiple: shows selection dialog
3. No matches: shows all attachments for manual selection

### Attachment Selection Dialog

- ListView with columns: **No** | **Date** | **Filename**
- Checkbox multi-select with **Select All** toggle
- Real-time text filter (case-insensitive filename search)
- **Double-click** any row to open that file immediately
- Status bar showing `Selected: X / Y`

### Runtime Settings

Accessible from **Plugins** > **jira-log-viewer** > **Settings**. Changes take effect immediately and persist across sessions in an INI file.

| Setting | Default | Description |
|---------|---------|-------------|
| Focused Extension | `.log` | File extension to prioritize when filtering attachments or extracted files |
| Archive Extension | `.7z` | Archive extension to look for in the archive priority path |
| Archive Prefix | `Pattern_` | Only archives whose filename starts with this prefix are considered (empty = all) |
| Search recursively | Off | Search subdirectories after extraction, or top-level only |

### Other

- Configurable keyboard shortcut (default `Ctrl+Shift+J`)
- `.7z` extraction via 7-Zip with stderr capture for error reporting
- HTTPS self-signed certificate bypass for corporate environments
- JRASERVER-72019 compatibility: detects PAT redirect-to-login and shows a clear error
- HTTP error handling: 401/403 (PAT issue), 404 (invalid Jira Key), timeout (30s connect / 60s receive)

## Requirements

- Windows 10+ (64-bit)
- Notepad++ 64-bit
- Visual Studio 2022 (v143 toolset) for building
- [7-Zip](https://www.7-zip.org/) installed (for `.7z` extraction)
- Jira Server/DC instance with PAT enabled

## Build

1. Open `jira-log-viewer.sln` in Visual Studio 2022
2. Edit `config/JiraConfig.h` with your Jira URL and PAT (see below)
3. Select **Release | x64**
4. Build Solution (`Ctrl+Shift+B`)
5. Output: `x64\Release\jira-log-viewer.dll`

## Install

1. In Notepad++ plugins directory (e.g. `C:\Program Files\Notepad++\plugins\`), create a folder named `jira-log-viewer`
2. Copy `jira-log-viewer.dll` into that folder
3. Restart Notepad++
4. The plugin appears under **Plugins** > **jira-log-viewer** with 3 menu items:
   - **Open Jira Attachment** (`Ctrl+Shift+J`)
   - **Settings**
   - **About**

## Configuration

### Compile-time (config/JiraConfig.h)

These require rebuilding the DLL to change.

```cpp
// Jira Server base URL (no trailing slash)
constexpr wchar_t JIRA_URL[] = L"https://jira.yourcompany.com";

// Personal Access Token for Jira Server/DC
constexpr wchar_t JIRA_PAT[] = L"your-personal-access-token";

// Path to 7-Zip command-line tool
constexpr wchar_t SEVEN_ZIP_PATH[] = L"C:\\Program Files\\7-Zip\\7z.exe";

// Shortcut key
constexpr bool  SHORTCUT_CTRL  = true;
constexpr bool  SHORTCUT_ALT   = false;
constexpr bool  SHORTCUT_SHIFT = true;
constexpr UCHAR SHORTCUT_KEY   = 'J';
```

The file also contains default values for the runtime settings (`FOCUSED_EXTENSION`, `FOCUSED_ARCHIVE_EXT`, `FOCUSED_PREFIX`, `FOCUSED_RECURSIVE`), which are used on first launch when no INI file exists.

### Runtime (Settings dialog)

The Settings dialog writes to `jira-log-viewer.ini` in the Notepad++ plugin config directory (obtained via `NPPM_GETPLUGINSCONFIGDIR`). Changes apply immediately without restarting.

## Usage

1. Press `Ctrl+Shift+J` (or your configured shortcut)
2. Enter a Jira Key (e.g. `PROJ-123`)
3. The plugin fetches attachments and applies smart filtering:
   - Matching archive found → downloads, extracts, finds log files inside
   - No archive → looks for direct log file attachments
   - Single result opens automatically; multiple results show a selection dialog
4. Double-click any row or check items and click Open
5. Files open as new tabs in Notepad++

## Project Structure

```
jira-log-viewer/
├── config/
│   ├── AboutInfo.h              # Plugin name, version, About dialog content
│   └── JiraConfig.h             # Jira URL, PAT, 7z path, shortcut, filter defaults
├── external/
│   ├── PluginInterface.h        # Notepad++ plugin SDK (NppData, FuncItem, ShortcutKey)
│   ├── Scintilla.h              # SCNotification struct
│   └── nlohmann/
│       └── json.hpp             # nlohmann/json v3.11.3 (header-only)
├── src/
│   ├── dllmain.cpp              # DLL entry point (stores g_hInstance)
│   ├── Plugin.h / .cpp          # 6 DLL exports, 3 menu commands, main workflow
│   ├── JiraClient.h / .cpp      # WinHTTP REST client (Bearer PAT, self-signed certs)
│   ├── FileHandler.h / .cpp     # Download to %TEMP%, .7z extraction, file enumeration
│   ├── InputDlg.h / .cpp        # Jira Key input dialog (custom WndProc)
│   ├── AttachmentDlg.h / .cpp   # Attachment/file selection dialog (ListView)
│   ├── Settings.h / .cpp        # INI load/save, runtime PluginSettings struct
│   └── SettingsDlg.h / .cpp     # Settings dialog UI
├── jira-log-viewer.sln          # VS 2022 solution (Debug|x64 + Release|x64)
├── jira-log-viewer.vcxproj      # v143, C++17, Unicode, x64
├── jira-log-viewer.vcxproj.filters
└── jira-log-viewer.def          # DLL exports (6 functions)
```

## Technical Details

| Component | Implementation |
|-----------|---------------|
| Build | VS 2022, v143, C++17, Unicode, x64 |
| HTTP | WinHTTP (Windows built-in), self-signed cert bypass via `WINHTTP_OPTION_SECURITY_FLAGS` |
| Auth | Bearer PAT, redirect detection for JRASERVER-72019 |
| JSON | nlohmann/json header-only (`external/nlohmann/json.hpp`) |
| Archive | `CreateProcess` → `7z.exe x -o"..." -y "..."`, stderr pipe capture |
| Dialogs | Programmatic Win32 (`CreateWindowExW` + custom WndProc), no `.rc` resources |
| Settings | `WritePrivateProfileString` / `GetPrivateProfileString`, config dir via `NPPM_GETPLUGINSCONFIGDIR` |
| Timeouts | 30s connect, 60s receive |
| Libraries | `winhttp.lib`, `comctl32.lib`, `shlwapi.lib` |

## Troubleshooting

| Symptom | Cause | Solution |
|---------|-------|----------|
| "401 Unauthorized" or "403 Forbidden" | Invalid or expired PAT | Update `JIRA_PAT` in `JiraConfig.h` and rebuild |
| "404 Not Found" | Jira Key does not exist | Verify the issue key (e.g. `PROJ-123`) |
| "302 redirect to /login detected" | Jira Server < 8.20 PAT limitation (JRASERVER-72019) | Upgrade Jira Server or use a different auth method |
| "7z.exe not found" | 7-Zip not installed or wrong path | Install 7-Zip or update `SEVEN_ZIP_PATH` in `JiraConfig.h` |
| Timeout / no response | Network issue or Jira server down | Check network connectivity; timeouts are 30s connect / 60s receive |
