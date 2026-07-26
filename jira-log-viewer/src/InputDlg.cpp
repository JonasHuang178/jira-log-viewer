#include "InputDlg.h"

static std::wstring s_inputResult;
static bool s_running = false;
static HWND s_hEdit = nullptr;

static LRESULT CALLBACK InputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        if (id == IDOK) {
            int len = ::GetWindowTextLengthW(s_hEdit);
            if (len > 0) {
                s_inputResult.resize(len);
                ::GetWindowTextW(s_hEdit, s_inputResult.data(), len + 1);
                s_running = false;
            }
            return 0;
        }
        if (id == IDCANCEL) {
            s_inputResult.clear();
            s_running = false;
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        s_inputResult.clear();
        s_running = false;
        return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

std::wstring ShowInputDlg(HINSTANCE /*hInst*/, HWND hParent)
{
    s_inputResult.clear();
    s_running = true;

    static bool registered = false;
    static const wchar_t* className = L"JiraLogViewerInputDlg";

    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = InputWndProc;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = className;
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    HWND hDlg = ::CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        className, L"Open Jira Attachment",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 350, 120,
        hParent, nullptr, ::GetModuleHandleW(nullptr), nullptr);

    RECT rcParent, rcDlg;
    ::GetWindowRect(hParent, &rcParent);
    ::GetWindowRect(hDlg, &rcDlg);
    int x = rcParent.left + ((rcParent.right - rcParent.left) - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcDlg.bottom - rcDlg.top)) / 2;
    ::SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    HFONT hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));

    HWND hLabel = ::CreateWindowExW(0, L"STATIC", L"Jira Key:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 14, 55, 16, hDlg, nullptr, nullptr, nullptr);
    ::SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    s_hEdit = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        70, 12, 170, 22, hDlg, reinterpret_cast<HMENU>(101), nullptr, nullptr);
    ::SendMessageW(s_hEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hBtnOk = ::CreateWindowExW(0, L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        250, 10, 70, 26, hDlg, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
    ::SendMessageW(hBtnOk, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hBtnCancel = ::CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        250, 44, 70, 26, hDlg, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
    ::SendMessageW(hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    ::EnableWindow(hParent, FALSE);
    ::ShowWindow(hDlg, SW_SHOW);
    ::SetFocus(s_hEdit);

    MSG msg;
    while (s_running && ::GetMessageW(&msg, nullptr, 0, 0))
    {
        if (msg.hwnd == hDlg || ::IsChild(hDlg, msg.hwnd))
        {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
                int len = ::GetWindowTextLengthW(s_hEdit);
                if (len > 0) {
                    s_inputResult.resize(len);
                    ::GetWindowTextW(s_hEdit, s_inputResult.data(), len + 1);
                }
                s_running = false;
                continue;
            }
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                s_inputResult.clear();
                s_running = false;
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
    if (::IsWindow(hDlg))
        ::DestroyWindow(hDlg);

    s_hEdit = nullptr;
    return s_inputResult;
}
