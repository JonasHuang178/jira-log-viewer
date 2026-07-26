## 1. Project Setup

- [x] 1.1 Create jira-log-viewer.sln（VS solution，Debug|x64 + Release|x64）
- [x] 1.2 Create jira-log-viewer.vcxproj（v143 toolset, x64, C++17, Unicode, include paths: external;src;config, link: winhttp.lib;comctl32.lib;shlwapi.lib）
- [x] 1.3 Create jira-log-viewer.vcxproj.filters（3 個 filter: external, config, src）
- [x] 1.4 Create jira-log-viewer.def（LIBRARY jira-log-viewer, exports: isUnicode, getName, getFuncsArray, setInfo, beNotified, messageProc）
- [x] 1.5 Create external/PluginInterface.h（Notepad++ plugin SDK：NppData, FuncItem, ShortcutKey 結構）
- [x] 1.6 Create external/Scintilla.h（SCNotification 結構 + 必要常數）
- [x] 1.7 Download nlohmann/json.hpp to external/nlohmann/json.hpp

## 2. Config Headers

- [x] 2.1 Create config/AboutInfo.h（PLUGIN_VERSION, PLUGIN_NAME, ABOUT_TITLE, ABOUT_CONTENT）
- [x] 2.2 Create config/JiraConfig.h（JIRA_URL, JIRA_PAT, SEVEN_ZIP_PATH, 快捷鍵 SHORTCUT_*, 過濾設定 FOCUSED_EXTENSION, FOCUSED_ARCHIVE_EXT, FOCUSED_PREFIX, FOCUSED_RECURSIVE）

## 3. Plugin Core

- [x] 3.1 Create src/dllmain.cpp（儲存 g_hInstance，不做其他事）
- [x] 3.2 Create src/Plugin.h（宣告 g_nppData, g_hInstance, GetCurrentScintilla）
- [x] 3.3 Create src/Plugin.cpp — 6 個 DLL exports（extern "C" 區塊），3 個 menu items：Open Jira Attachment（快捷鍵從 JiraConfig.h 讀取，預設 Ctrl+Shift+J）+ Settings + About
- [x] 3.4 Implement ShowAbout()（使用 config/AboutInfo.h 的 ABOUT_TITLE 和 ABOUT_CONTENT）
- [x] 3.5 Implement OpenSettings()（呼叫 ShowSettingsDlg）

## 4. Jira API Client

- [x] 4.1 Create src/JiraClient.h — JiraAttachment struct（id, filename, size, created, author, contentUrl）+ JiraClient class 宣告
- [x] 4.2 Create src/JiraClient.cpp — WinHTTP session 初始化 + self-signed cert handling
- [x] 4.3 Implement getAttachments()（GET /rest/api/2/issue/{key}, parse JSON fields.attachment[], return vector + issue title）
- [x] 4.4 Implement downloadFile()（GET content URL, write binary to local path）
- [x] 4.5 Handle HTTP errors: 401/403 → 提示 PAT, 404 → Jira Key 不存在, 302 /login → JRASERVER-72019
- [x] 4.6 Set timeouts: connect 30s, receive 60s

## 5. Dialogs

- [x] 5.1 Create src/InputDlg.h/.cpp — 程式碼建立 modal dialog（350x120, 自訂 InputWndProc），文字輸入 Jira Key，OK/Cancel
- [x] 5.2 Create src/AttachmentDlg.h/.cpp — 程式碼建立 modal dialog，ListView (Report + checkbox)
- [x] 5.3 AttachmentDlg columns: No, Date, Filename（附 checkbox + 雙擊開啟 NM_DBLCLK）
- [x] 5.4 AttachmentDlg: text filter（即時過濾 filename，case-insensitive）
- [x] 5.5 AttachmentDlg: Select All toggle button
- [x] 5.6 AttachmentDlg: status text 顯示 "Selected: X / Y"
- [x] 5.7 AttachmentDlg: dialog title 顯示 Jira Key + issue summary
- [x] 5.8 AttachmentDlg: 複用同一個 dialog 給 .7z 解壓後的檔案選擇

## 6. File Handler

- [x] 6.1 Create src/FileHandler.h/.cpp — download dir 管理（%TEMP%\jira-log-viewer\{JIRA-KEY}\）
- [x] 6.2 Implement .7z 偵測 + 解壓（CreateProcess 呼叫 7z.exe，output dir 為 archive name 去掉副檔名的子目錄，支援 recursive 參數控制檔案列舉深度）
- [x] 6.3 Handle 7z.exe not found 錯誤 + extraction failure（capture stderr）
- [x] 6.4 Post-extraction: single file → 直接開；multiple files → show AttachmentDlg
- [x] 6.5 Open files in Notepad++ via NPPM_DOOPEN

## 7. Main Workflow Integration

- [x] 7.1 Wire OpenJiraAttachment: input dialog → API call → smart filter → download → extract → filter → open（寫在 Plugin.cpp 內）
- [x] 7.2 Show wait cursor (IDC_WAIT) during API calls and downloads
- [x] 7.3 Handle empty attachment list（顯示 info message）
- [x] 7.4 Implement Path A: 壓縮檔路徑（FOCUSED_ARCHIVE_EXT + FOCUSED_PREFIX 過濾 → 自動選擇或對話框 → 解壓 → FOCUSED_EXTENSION 過濾 → 開啟）
- [x] 7.5 Implement Path B: 直接檔案路徑（FOCUSED_EXTENSION 過濾 → 自動選擇或對話框 → 下載開啟）
- [x] 7.6 OpenJiraAttachment 改用 GetSettings() runtime 設定取代 constexpr 常數

## 8. Build and Verify

- [x] 8.1 Open jira-log-viewer.sln in VS 2022, verify build succeeds (Debug + Release x64)
- [ ] 8.2 Copy jira-log-viewer.dll to Notepad++ plugins directory, verify plugin loads
- [ ] 8.3 Verify menu appears, shortcut key works, About dialog displays correctly

## 9. Settings（Runtime 設定）

- [x] 9.1 Create src/Settings.h/.cpp — PluginSettings 結構 + InitSettings/SaveSettings/GetSettings
- [x] 9.2 INI 讀寫：NPPM_GETPLUGINSCONFIGDIR + WritePrivateProfileString/GetPrivateProfileString
- [x] 9.3 Create src/SettingsDlg.h/.cpp — Settings 對話框 UI（4 欄位 + OK/Cancel）
- [x] 9.4 Plugin.cpp: setInfo() 中呼叫 InitSettings()，OpenJiraAttachment() 使用 GetSettings()
- [x] 9.5 vcxproj 新增 Settings.cpp/.h 和 SettingsDlg.cpp/.h

## 10. Bug Fixes（實作階段修正）

- [x] 10.1 Fix .def LIBRARY name: `JiraLogViewer` → `jira-log-viewer`（修正 LNK4070 warning）
- [x] 10.2 Fix InputDlg window height: 90px → 120px（Cancel 按鈕被裁切）
- [x] 10.3 Fix InputDlg WndProc: `DefWindowProcW` → 自訂 `InputWndProc`（按鈕 WM_COMMAND 不會被轉發到 DefWindowProcW）
- [x] 10.4 Add shortcut key configurability: JiraConfig.h 新增 SHORTCUT_CTRL/ALT/SHIFT/KEY 常數，Plugin.cpp 從常數設定 ShortcutKey
- [x] 10.5 Fix vector<bool> proxy reference: NM_DBLCLK handler 中 `for (auto& c : checked)` → `std::fill`
