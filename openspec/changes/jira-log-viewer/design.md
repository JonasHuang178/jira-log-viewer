## Context

使用者在公司內部使用 Jira Server/DC 管理問題追蹤，每個 ticket 會上傳多個 log 檔案（.log、.7z）作為附件。目前分析 log 的流程需要手動從 Jira 網頁下載附件再用 Notepad++ 開啟，操作繁瑣。

本 plugin 為全新專案。目標環境為 Windows 10+、Notepad++ 64-bit、公司內部 Jira Server（HTTPS + 自簽憑證）。

## Goals / Non-Goals

**Goals:**

- 在 Notepad++ 內一鍵完成「輸入 Jira Key → 選擇附件 → 下載 → 開啟」的完整流程
- 支援 .7z 壓縮檔自動解壓後選擇檔案開啟
- 支援 HTTPS 自簽憑證環境
- 零外部 runtime 依賴（僅需 Windows 內建 API + 已安裝的 7-Zip）

**Non-Goals:**

- 附件過濾設定提供 GUI 介面（Settings 對話框 + INI 持久化），Jira 連線設定仍為編譯時決定
- 不支援 Jira Cloud（僅 Server/DC）
- 不支援 Basic Auth（僅 PAT）
- 不做 log 分析或語法高亮
- 不做附件上傳功能
- 不做快取機制（每次重新下載）

## Decisions

### 1. 專案結構

```
jira-log-viewer/
├── config/
│   ├── AboutInfo.h            ← plugin 名稱/版本/About 內容
│   └── JiraConfig.h           ← JIRA_URL, JIRA_PAT, SEVEN_ZIP_PATH, 快捷鍵設定
├── external/
│   ├── PluginInterface.h      ← Notepad++ plugin SDK
│   ├── Scintilla.h            ← Scintilla 編輯器介面
│   └── nlohmann/
│       └── json.hpp           ← header-only JSON library
├── src/
│   ├── dllmain.cpp            ← DLL 進入點
│   ├── Plugin.h / .cpp        ← 6 個 DLL exports + 指令入口
│   ├── JiraClient.h / .cpp    ← WinHTTP REST API client
│   ├── FileHandler.h / .cpp   ← 下載、解壓、開啟
│   ├── InputDlg.h / .cpp      ← Jira Key 輸入對話框
│   ├── AttachmentDlg.h / .cpp ← 附件/檔案選擇對話框
│   ├── Settings.h / .cpp      ← INI 設定讀寫 + runtime 設定管理
│   └── SettingsDlg.h / .cpp   ← Settings 對話框 UI
├── jira-log-viewer.sln
├── jira-log-viewer.vcxproj
├── jira-log-viewer.vcxproj.filters
└── jira-log-viewer.def
```

目錄分為三層：`config/`（編譯時設定）、`external/`（第三方 headers）、`src/`（所有 source 平鋪）。

### 2. 建置系統 — VS .sln/.vcxproj

**選擇**：直接使用 Visual Studio solution + vcxproj

**替代方案**：CMake 生成 VS solution

**理由**：直接開 .sln 即可編譯，不需額外安裝 CMake。vcxproj 設定：v143 toolset、x64、C++17、Unicode。

### 3. DLL Export — .def 檔案

**選擇**：使用 `jira-log-viewer.def` 定義 exports

**替代方案**：`__declspec(dllexport)`

**理由**：`.def` 檔案讓 export 清單一目了然。vcxproj 中透過 `ModuleDefinitionFile` 引用。

### 4. 設定方式 — 雙層設定架構

設定分為兩層：

**編譯時設定（`config/` header 檔）**：
- `config/AboutInfo.h` — plugin 名稱、版本、About 對話框內容
- `config/JiraConfig.h` — Jira URL、PAT、7z.exe 路徑、快捷鍵、附件過濾預設值

`JiraConfig.h` 包含快捷鍵的 4 個 `constexpr` 常數：
```cpp
constexpr bool  SHORTCUT_CTRL  = true;
constexpr bool  SHORTCUT_ALT   = false;
constexpr bool  SHORTCUT_SHIFT = true;
constexpr UCHAR SHORTCUT_KEY   = 'J';
```

附件過濾預設值：
```cpp
constexpr wchar_t FOCUSED_EXTENSION[]    = L".log";
constexpr wchar_t FOCUSED_ARCHIVE_EXT[]  = L".7z";
constexpr wchar_t FOCUSED_PREFIX[]       = L"Pattern_";
constexpr bool    FOCUSED_RECURSIVE      = false;
```

**Runtime 設定（INI 檔案 + Settings 對話框）**：
- `src/Settings.h/.cpp` — 管理 `PluginSettings` 結構，負責 INI 讀寫
- `src/SettingsDlg.h/.cpp` — Settings 對話框 UI
- INI 路徑：`{Notepad++ plugin config dir}\jira-log-viewer.ini`
- 首次載入時使用 `JiraConfig.h` 的 `constexpr` 值作為 default
- 透過 `NPPM_GETPLUGINSCONFIGDIR` 取得 config 目錄
- 使用 `WritePrivateProfileString` / `GetPrivateProfileString` 讀寫

Runtime 可調設定（透過 Settings 對話框，立即生效）：
- `FocusedExtension` — 專注的副檔名（預設 `.log`）
- `FocusedArchiveExt` — 壓縮檔副檔名（預設 `.7z`）
- `FocusedPrefix` — 壓縮檔前綴過濾（預設 `Pattern_`）
- `FocusedRecursive` — 解壓後是否遞迴搜尋（預設 `false`）

**理由**：Jira 連線設定（URL、PAT）含敏感資訊，適合編譯時寫死。附件過濾行為因 ticket 類型不同可能需要頻繁調整，適合 runtime 修改。

### 5. Include 路徑

vcxproj 中設定：
```
AdditionalIncludeDirectories: $(ProjectDir)external;$(ProjectDir)src;$(ProjectDir)config
```

### 6. HTTP Client — WinHTTP

**選擇**：WinHTTP（Windows 內建）

**替代方案**：libcurl、cpp-httplib

**理由**：零外部依賴，內建 TLS 支援，可透過 `WINHTTP_OPTION_SECURITY_FLAGS` 處理自簽憑證。vcxproj 中只需在 `AdditionalDependencies` 加上 `winhttp.lib`。

### 7. JSON Parser — nlohmann/json

**選擇**：nlohmann/json（header-only），放在 `external/nlohmann/json.hpp`

**替代方案**：RapidJSON、手寫 parser

**理由**：API 回應量小，效能非瓶頸。header-only 直接放 `external/` 目錄即可，不需 CMake 或 package manager。

### 8. .7z 解壓 — 外部呼叫 7z.exe

**選擇**：`CreateProcess` 呼叫 `7z.exe x -o"..." -y "..."`

**替代方案**：bit7z library、7-Zip SDK（7z.dll）

**理由**：使用者已安裝 7-Zip，直接呼叫最簡單。`CreateProcess` 搭配 stdout/stderr pipe 即可取得執行結果。

### 9. 執行緒模型 — std::thread + 等待游標

**選擇**：API 呼叫在 std::thread 中執行，主執行緒顯示等待游標（IDC_WAIT），使用 `thread.join()` 等待完成。

**替代方案**：非同步 + 進度對話框

**理由**：附件清單 API 回應通常在 1-2 秒內完成，不需要複雜的非同步機制。WinHTTP 設定 timeout 避免永久卡住。

### 10. Plugin.cpp 程式碼慣例

- 全域變數用 `g_` 前綴（`g_nppData`, `g_hInstance`, `g_funcItems`）
- DLL exports 包在 `extern "C" { }` 區塊內
- `dllmain.cpp` 僅負責儲存 `g_hInstance`，不做其他事
- 指令函式（`OpenJiraAttachment`, `OpenSettings`, `ShowAbout`）為 `static` 函式，寫在 `Plugin.cpp` 內
- `setInfo()` 中呼叫 `InitSettings()` 載入 INI 設定

### 11. 對話框實作 — 程式碼建立 + 自訂 WndProc

**選擇**：所有對話框以 `CreateWindowExW` + 自訂 `WndProc` 建立，不使用 .rc 資源檔

**替代方案**：.rc dialog 資源 + `DialogBoxParam`

**理由**：不依賴 Resource Editor，程式碼中直接控制 layout。InputDlg 使用自訂 `InputWndProc` 處理 `WM_COMMAND`（OK/Cancel 按鈕事件），因為 `DefWindowProcW` 不轉發按鈕的 `BN_CLICKED` 通知。AttachmentDlg 也使用自訂 `WndProc` 處理 ListView 事件和按鈕操作。

### 12. 智慧附件過濾 — 兩段式流程

附件處理分為兩條路徑，依優先順序嘗試：

**Path A — 壓縮檔路徑**：
1. 篩選附件：副檔名 = `FOCUSED_ARCHIVE_EXT` 且檔名以 `FOCUSED_PREFIX` 開頭
2. 1 個符合 → 直接下載解壓；多個 → 顯示選擇對話框
3. 解壓後搜尋檔案（`FOCUSED_RECURSIVE` 控制是否遞迴）
4. 用 `FOCUSED_EXTENSION` 過濾解壓結果
5. 1 個符合 → 直接開；多個 → 顯示選擇對話框

**Path B — 直接檔案路徑**（Path A 無符合時）：
1. 篩選附件：副檔名 = `FOCUSED_EXTENSION`
2. 1 個符合 → 直接開；多個 → 顯示選擇對話框
3. 無符合 → 顯示所有附件讓使用者選擇

**理由**：壓縮檔路徑優先，因為大部分 ticket 的 log 都打包在 .7z 內。直接檔案路徑作為 fallback。

### 13. 附件選擇對話框欄位

**選擇**：ListView 欄位為 `No | Date | Filename`，支援雙擊開啟

**替代方案**：`Filename | Size | Date | Author`（原始設計）

**理由**：實際使用中，使用者主要依據日期和檔名選擇附件。序號方便識別位置，雙擊（`NM_DBLCLK`）提供快速開啟單一檔案。

## Risks / Trade-offs

- **[PAT 寫在 source code]** → 不適合公開 repo。此為個人使用工具，不需公開發布。若需分享，應改為 INI 或環境變數。
- **[自簽憑證忽略驗證]** → 降低安全性。僅在公司內部網路使用，風險可接受。
- **[7z.exe 路徑寫死]** → 若安裝在非預設路徑會失敗。啟動時檢查並提供明確錯誤訊息。
- **[thread.join() 阻塞 UI]** → 網路異常時 Notepad++ 可能暫時無回應。WinHTTP 設定 timeout（30 秒連線、60 秒接收）避免永久卡住。
- **[nlohmann/json.hpp 檔案大]** → 約 24,000 行 header，但只 include 一次（在 JiraClient.cpp），不影響整體編譯速度。
- **[Jira Server PAT 附件下載相容性]** → Jira Server 8.20 以前版本的 PAT 可能無法下載附件（JRASERVER-72019），需偵測 302 redirect 到 login 頁面並提供明確錯誤訊息。
