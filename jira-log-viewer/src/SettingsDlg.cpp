#include "SettingsDlg.h"
#include "Settings.h"

enum {
    IDC_EDIT_EXT     = 300,
    IDC_EDIT_ARC_EXT = 301,
    IDC_EDIT_PREFIX  = 302,
    IDC_CHK_RECURSE  = 303,
    IDC_BTN_OK       = 304,
    IDC_BTN_CANCEL   = 305,
};

static LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        if (id == IDC_BTN_OK) {
            auto& s = GetSettings();

            wchar_t buf[256] = {};
            ::GetWindowTextW(::GetDlgItem(hWnd, IDC_EDIT_EXT), buf, _countof(buf));
            s.focusedExtension = buf;

            ::GetWindowTextW(::GetDlgItem(hWnd, IDC_EDIT_ARC_EXT), buf, _countof(buf));
            s.focusedArchiveExt = buf;

            ::GetWindowTextW(::GetDlgItem(hWnd, IDC_EDIT_PREFIX), buf, _countof(buf));
            s.focusedPrefix = buf;

            s.focusedRecursive = ::SendDlgItemMessageW(hWnd, IDC_CHK_RECURSE, BM_GETCHECK, 0, 0) == BST_CHECKED;

            SaveSettings();

            ::EnableWindow(::GetParent(hWnd), TRUE);
            ::DestroyWindow(hWnd);
            return 0;
        }
        if (id == IDC_BTN_CANCEL || id == IDCANCEL) {
            ::EnableWindow(::GetParent(hWnd), TRUE);
            ::DestroyWindow(hWnd);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        ::EnableWindow(::GetParent(hWnd), TRUE);
        ::DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ShowSettingsDlg(HINSTANCE /*hInst*/, HWND hParent)
{
    static bool registered = false;
    static const wchar_t* className = L"JiraLogViewerSettingsDlg";

    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = SettingsWndProc;
        wc.hInstance      = ::GetModuleHandleW(nullptr);
        wc.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName  = className;
        wc.hCursor        = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    const int dlgW = 400, dlgH = 230;

    RECT rcParent;
    ::GetWindowRect(hParent, &rcParent);
    int x = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int y = rcParent.top  + ((rcParent.bottom - rcParent.top) - dlgH) / 2;

    HWND hDlg = ::CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        className, L"jira-log-viewer Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, dlgW, dlgH,
        hParent, nullptr, ::GetModuleHandleW(nullptr), nullptr);

    HFONT hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));

    auto makeLabel = [&](const wchar_t* text, int ly) {
        HWND h = ::CreateWindowExW(0, L"STATIC", text,
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            10, ly + 2, 140, 16, hDlg, nullptr, nullptr, nullptr);
        ::SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    };

    auto makeEdit = [&](int id, const std::wstring& val, int ey) -> HWND {
        HWND h = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", val.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            158, ey, 210, 22, hDlg, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
        ::SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        return h;
    };

    const auto& s = GetSettings();
    int row = 14;

    makeLabel(L"Focused Extension:", row);
    makeEdit(IDC_EDIT_EXT, s.focusedExtension, row);

    row += 30;
    makeLabel(L"Archive Extension:", row);
    makeEdit(IDC_EDIT_ARC_EXT, s.focusedArchiveExt, row);

    row += 30;
    makeLabel(L"Archive Prefix:", row);
    makeEdit(IDC_EDIT_PREFIX, s.focusedPrefix, row);

    row += 34;
    HWND hChk = ::CreateWindowExW(0, L"BUTTON", L"Search extracted files recursively",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        158, row, 220, 20, hDlg, reinterpret_cast<HMENU>(IDC_CHK_RECURSE), nullptr, nullptr);
    ::SendMessageW(hChk, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    ::SendMessageW(hChk, BM_SETCHECK, s.focusedRecursive ? BST_CHECKED : BST_UNCHECKED, 0);

    row += 34;
    int btnW = 80, btnH = 26;
    HWND hOk = ::CreateWindowExW(0, L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        dlgW - 2 * (btnW + 10) - 10, row, btnW, btnH,
        hDlg, reinterpret_cast<HMENU>(IDC_BTN_OK), nullptr, nullptr);
    ::SendMessageW(hOk, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hCancel = ::CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        dlgW - (btnW + 20), row, btnW, btnH,
        hDlg, reinterpret_cast<HMENU>(IDC_BTN_CANCEL), nullptr, nullptr);
    ::SendMessageW(hCancel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    ::EnableWindow(hParent, FALSE);
    ::ShowWindow(hDlg, SW_SHOW);

    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.hwnd == hDlg || ::IsChild(hDlg, msg.hwnd)) {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                ::EnableWindow(hParent, TRUE);
                ::DestroyWindow(hDlg);
                continue;
            }
        }
        if (!::IsDialogMessageW(hDlg, &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    ::EnableWindow(hParent, TRUE);
    ::SetForegroundWindow(hParent);
}
