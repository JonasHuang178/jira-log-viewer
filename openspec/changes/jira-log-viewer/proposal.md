## Why

每次分析 Jira ticket 上的 log 附件，都需要手動進入 Jira 網頁 → 找到附件 → 下載 → 用 Notepad++ 開啟。當 ticket 有多個 log 或 .7z 壓縮檔時，重複操作非常耗時。需要一個 Notepad++ plugin，讓使用者直接在編輯器中輸入 Jira Key 就能一鍵取得並開啟 log 檔案。

## What Changes

- 新增 Notepad++ C++ plugin（DLL），名稱為 `jira-log-viewer`
- 提供可設定的快捷鍵（預設 Ctrl+Shift+J）觸發主功能，透過 `config/JiraConfig.h` 設定
- 透過 Jira REST API（Bearer PAT 認證）取得 ticket 附件列表
- 智慧附件過濾：優先處理符合前綴+副檔名的壓縮檔（預設 `Pattern_*.7z`），解壓後自動搜尋目標檔案（預設 `.log`）
- 附件選擇對話框（ListView），欄位 No | Date | Filename，支援文字過濾、Select All、雙擊開啟
- 自動下載附件至 `%TEMP%\jira-log-viewer\{JIRA-KEY}\`
- 支援 .7z 自動解壓（呼叫 7z.exe），解壓後可遞迴或僅搜尋頂層
- 單一符合檔案自動開啟，多個檔案才顯示選擇對話框
- 直接在 Notepad++ 中開啟選定的 log 檔案
- 支援 HTTPS 自簽憑證（公司 Jira Server/DC 環境）
- 編譯時設定（Jira URL、PAT、7z 路徑、快捷鍵）寫在 `config/JiraConfig.h`
- Runtime 設定（副檔名過濾、壓縮檔前綴、遞迴搜尋）透過 Settings 對話框修改，存在 INI 檔案，立刻生效

## Capabilities

### New Capabilities

- `jira-api-client`: 透過 WinHTTP 呼叫 Jira REST API，支援 Bearer PAT 認證與自簽憑證
- `attachment-browser`: 附件選擇對話框（No | Date | Filename），支援過濾、多選、雙擊開啟
- `file-download-extract`: 下載附件至暫存目錄，支援 .7z 自動解壓（可設定遞迴搜尋），智慧過濾目標副檔名
- `npp-plugin-shell`: Notepad++ plugin DLL 骨架，包含選單（Open Jira Attachment / Settings / About）、快捷鍵與檔案開啟整合
- `settings-dialog`: Runtime 設定對話框 + INI 檔案持久化，修改後立刻生效

### Modified Capabilities

（無，此為全新 plugin）

## Impact

- **新增 DLL**：`jira-log-viewer.dll`，安裝至 Notepad++ plugins 目錄
- **新增 INI**：`jira-log-viewer.ini`，自動建立於 Notepad++ plugin config 目錄
- **外部依賴**：WinHTTP（Windows 內建）、nlohmann/json（header-only，放在 `external/nlohmann/json.hpp`）、7-Zip（使用者自行安裝）
- **網路存取**：需連線至公司 Jira Server（HTTPS + 自簽憑證）
- **建置需求**：Visual Studio 2022（v143 toolset）、Windows SDK
