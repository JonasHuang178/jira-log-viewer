#include "AttachmentDlg.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// ---------------------------------------------------------------------------
// Control IDs
// ---------------------------------------------------------------------------
enum {
    IDC_FILTER_EDIT = 200,
    IDC_LISTVIEW    = 201,
    IDC_STATUS      = 202,
    IDC_BTN_SELALL  = 203,
    IDC_BTN_OPEN    = 204,
    IDC_BTN_CANCEL  = 205,
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::wstring FormatSize(int64_t bytes)
{
    wchar_t buf[64];
    if (bytes >= 1024LL * 1024 * 1024)
        ::swprintf_s(buf, L"%.1f GB", bytes / (1024.0 * 1024 * 1024));
    else if (bytes >= 1024LL * 1024)
        ::swprintf_s(buf, L"%.1f MB", bytes / (1024.0 * 1024));
    else if (bytes >= 1024)
        ::swprintf_s(buf, L"%.1f KB", bytes / 1024.0);
    else
        ::swprintf_s(buf, L"%lld B", bytes);
    return buf;
}

static std::wstring FormatDate(const std::wstring& isoDate)
{
    // "2024-01-15T10:30:00.000+0000" → "2024-01-15 10:30"
    if (isoDate.size() >= 16) {
        std::wstring d = isoDate.substr(0, 10) + L" " + isoDate.substr(11, 5);
        return d;
    }
    return isoDate;
}

static bool ContainsCI(const std::wstring& str, const std::wstring& sub)
{
    if (sub.empty()) return true;
    auto it = std::search(str.begin(), str.end(), sub.begin(), sub.end(),
        [](wchar_t a, wchar_t b) { return ::towlower(a) == ::towlower(b); });
    return it != str.end();
}

// ---------------------------------------------------------------------------
// Attachment Dialog state
// ---------------------------------------------------------------------------
struct AttDlgState {
    const std::vector<JiraAttachment>* allItems = nullptr;
    std::vector<size_t> visibleIndices;
    std::vector<bool>   checked;
    HWND hList   = nullptr;
    HWND hFilter = nullptr;
    HWND hStatus = nullptr;
    bool confirmed = false;
};

static void RefreshList(AttDlgState& st, const std::wstring& filter)
{
    ListView_DeleteAllItems(st.hList);
    st.visibleIndices.clear();

    for (size_t i = 0; i < st.allItems->size(); ++i)
    {
        if (!ContainsCI((*st.allItems)[i].filename, filter))
            continue;
        st.visibleIndices.push_back(i);

        int row = static_cast<int>(st.visibleIndices.size() - 1);
        const auto& att = (*st.allItems)[i];

        wchar_t numBuf[16];
        ::swprintf_s(numBuf, L"%d", row + 1);

        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = row;
        lvi.iSubItem = 0;
        lvi.pszText = numBuf;
        ListView_InsertItem(st.hList, &lvi);

        std::wstring date = FormatDate(att.created);
        ListView_SetItemText(st.hList, row, 1, const_cast<LPWSTR>(date.c_str()));

        ListView_SetItemText(st.hList, row, 2, const_cast<LPWSTR>(att.filename.c_str()));

        ListView_SetCheckState(st.hList, row, st.checked[i] ? TRUE : FALSE);
    }
}

static void UpdateStatus(AttDlgState& st)
{
    int total = static_cast<int>(st.allItems->size());
    int sel = 0;
    for (bool c : st.checked) if (c) ++sel;

    wchar_t buf[64];
    ::swprintf_s(buf, L"Selected: %d / %d", sel, total);
    ::SetWindowTextW(st.hStatus, buf);

    ::EnableWindow(::GetDlgItem(::GetParent(st.hList), IDC_BTN_OPEN), sel > 0);
}

static void SyncChecksFromListView(AttDlgState& st)
{
    for (int row = 0; row < static_cast<int>(st.visibleIndices.size()); ++row) {
        size_t idx = st.visibleIndices[row];
        st.checked[idx] = ListView_GetCheckState(st.hList, row) != 0;
    }
}

static LRESULT CALLBACK AttDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* st = reinterpret_cast<AttDlgState*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE:
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        st = reinterpret_cast<AttDlgState*>(cs->lpCreateParams);
        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
        return 0;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDC_FILTER_EDIT && code == EN_CHANGE) {
            SyncChecksFromListView(*st);
            wchar_t filter[256] = {};
            ::GetWindowTextW(st->hFilter, filter, _countof(filter));
            RefreshList(*st, filter);
            UpdateStatus(*st);
            return 0;
        }

        if (id == IDC_BTN_SELALL) {
            SyncChecksFromListView(*st);
            bool allChecked = true;
            for (auto idx : st->visibleIndices) {
                if (!st->checked[idx]) { allChecked = false; break; }
            }
            for (auto idx : st->visibleIndices)
                st->checked[idx] = !allChecked;
            wchar_t filter[256] = {};
            ::GetWindowTextW(st->hFilter, filter, _countof(filter));
            RefreshList(*st, filter);
            UpdateStatus(*st);
            return 0;
        }

        if (id == IDC_BTN_OPEN) {
            SyncChecksFromListView(*st);
            st->confirmed = true;
            ::PostMessageW(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }

        if (id == IDC_BTN_CANCEL || id == IDCANCEL) {
            st->confirmed = false;
            ::PostMessageW(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    }

    case WM_NOTIFY:
    {
        auto* nmhdr = reinterpret_cast<NMHDR*>(lParam);
        if (nmhdr->idFrom == IDC_LISTVIEW) {
            if (nmhdr->code == LVN_ITEMCHANGED) {
                auto* nmlv = reinterpret_cast<NMLISTVIEW*>(lParam);
                if ((nmlv->uChanged & LVIF_STATE) &&
                    ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_STATEIMAGEMASK)) {
                    SyncChecksFromListView(*st);
                    UpdateStatus(*st);
                }
            }
            else if (nmhdr->code == NM_DBLCLK) {
                auto* nmia = reinterpret_cast<NMITEMACTIVATE*>(lParam);
                if (nmia->iItem >= 0 && nmia->iItem < static_cast<int>(st->visibleIndices.size())) {
                    std::fill(st->checked.begin(), st->checked.end(), false);
                    st->checked[st->visibleIndices[nmia->iItem]] = true;
                    st->confirmed = true;
                    ::PostMessageW(hWnd, WM_CLOSE, 0, 0);
                }
            }
        }
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            st->confirmed = false;
            ::PostMessageW(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_SIZE:
    {
        RECT rc;
        ::GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        int btnW = 80, btnH = 26, pad = 10;

        ::MoveWindow(st->hFilter, 50, pad, 200, 20, TRUE);
        ::MoveWindow(st->hList, pad, 36, w - 2 * pad, h - 80, TRUE);
        ::MoveWindow(st->hStatus, pad, h - 36, 200, 16, TRUE);

        int btnY = h - 40;
        ::MoveWindow(::GetDlgItem(hWnd, IDC_BTN_SELALL), w - 3 * (btnW + 6), btnY, btnW, btnH, TRUE);
        ::MoveWindow(::GetDlgItem(hWnd, IDC_BTN_OPEN), w - 2 * (btnW + 6), btnY, btnW, btnH, TRUE);
        ::MoveWindow(::GetDlgItem(hWnd, IDC_BTN_CANCEL), w - (btnW + pad), btnY, btnW, btnH, TRUE);
        return 0;
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

// ---------------------------------------------------------------------------
// ShowAttachmentDlg
// ---------------------------------------------------------------------------
std::vector<JiraAttachment> ShowAttachmentDlg(
    HINSTANCE /*hInst*/,
    HWND hParent,
    const std::wstring& jiraKey,
    const std::wstring& issueTitle,
    const std::vector<JiraAttachment>& attachments)
{
    static bool registered = false;
    static const wchar_t* className = L"JiraLogViewerAttDlg";

    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = AttDlgProc;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = className;
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&wc);
        registered = true;
    }

    AttDlgState state;
    state.allItems = &attachments;
    state.checked.resize(attachments.size(), false);

    std::wstring title = jiraKey;
    if (!issueTitle.empty())
        title += L" - " + issueTitle;

    RECT rcParent;
    ::GetWindowRect(hParent, &rcParent);
    int dlgW = 650, dlgH = 450;
    int x = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;

    HWND hWnd = ::CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className, title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        x, y, dlgW, dlgH,
        hParent, nullptr, ::GetModuleHandleW(nullptr), &state);

    HFONT hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));

    HWND hLabel = ::CreateWindowExW(0, L"STATIC", L"Filter:",
        WS_CHILD | WS_VISIBLE, 10, 12, 38, 16, hWnd, nullptr, nullptr, nullptr);
    ::SendMessageW(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    state.hFilter = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        50, 10, 200, 20, hWnd, reinterpret_cast<HMENU>(IDC_FILTER_EDIT), nullptr, nullptr);
    ::SendMessageW(state.hFilter, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    state.hList = ::CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        LVS_REPORT | LVS_SHOWSELALWAYS,
        10, 36, dlgW - 30, dlgH - 100,
        hWnd, reinterpret_cast<HMENU>(IDC_LISTVIEW), nullptr, nullptr);
    ListView_SetExtendedListViewStyle(state.hList,
        LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    ::SendMessageW(state.hList, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Columns
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.cx = 45;  col.pszText = const_cast<LPWSTR>(L"No");       col.iSubItem = 0;
    ListView_InsertColumn(state.hList, 0, &col);
    col.cx = 140; col.pszText = const_cast<LPWSTR>(L"Date");     col.iSubItem = 1;
    ListView_InsertColumn(state.hList, 1, &col);
    col.cx = 380; col.pszText = const_cast<LPWSTR>(L"Filename"); col.iSubItem = 2;
    ListView_InsertColumn(state.hList, 2, &col);

    state.hStatus = ::CreateWindowExW(0, L"STATIC", L"Selected: 0 / 0",
        WS_CHILD | WS_VISIBLE, 10, dlgH - 56, 200, 16,
        hWnd, reinterpret_cast<HMENU>(IDC_STATUS), nullptr, nullptr);
    ::SendMessageW(state.hStatus, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    int btnW = 80, btnH = 26;
    HWND hBtnSA = ::CreateWindowExW(0, L"BUTTON", L"Select All",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        dlgW - 3 * (btnW + 6) - 10, dlgH - 60, btnW, btnH,
        hWnd, reinterpret_cast<HMENU>(IDC_BTN_SELALL), nullptr, nullptr);
    ::SendMessageW(hBtnSA, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hBtnOpen = ::CreateWindowExW(0, L"BUTTON", L"Open",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        dlgW - 2 * (btnW + 6) - 10, dlgH - 60, btnW, btnH,
        hWnd, reinterpret_cast<HMENU>(IDC_BTN_OPEN), nullptr, nullptr);
    ::SendMessageW(hBtnOpen, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hBtnCancel = ::CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        dlgW - (btnW + 16), dlgH - 60, btnW, btnH,
        hWnd, reinterpret_cast<HMENU>(IDC_BTN_CANCEL), nullptr, nullptr);
    ::SendMessageW(hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    RefreshList(state, L"");
    UpdateStatus(state);

    ::EnableWindow(hParent, FALSE);
    ::ShowWindow(hWnd, SW_SHOW);

    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        if (!::IsDialogMessageW(hWnd, &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    ::EnableWindow(hParent, TRUE);
    ::SetForegroundWindow(hParent);

    std::vector<JiraAttachment> result;
    if (state.confirmed) {
        for (size_t i = 0; i < attachments.size(); ++i) {
            if (state.checked[i])
                result.push_back(attachments[i]);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// ShowFileDlg — reuses same UI for extracted file selection
// ---------------------------------------------------------------------------
std::vector<std::wstring> ShowFileDlg(
    HINSTANCE /*hInst*/,
    HWND hParent,
    const std::wstring& jiraKey,
    const std::wstring& archiveName,
    const std::vector<std::wstring>& files)
{
    // Convert file paths to JiraAttachment format for reuse
    std::vector<JiraAttachment> fakeAtts;
    for (const auto& f : files) {
        JiraAttachment a;
        // Extract filename from full path
        const wchar_t* name = ::PathFindFileNameW(f.c_str());
        a.filename = name;
        a.contentUrl = f;

        WIN32_FILE_ATTRIBUTE_DATA fad = {};
        if (::GetFileAttributesExW(f.c_str(), GetFileExInfoStandard, &fad)) {
            LARGE_INTEGER li;
            li.HighPart = fad.nFileSizeHigh;
            li.LowPart = fad.nFileSizeLow;
            a.size = li.QuadPart;
        }

        fakeAtts.push_back(std::move(a));
    }

    std::wstring title = jiraKey + L" - " + archiveName + L" (extracted)";
    auto selected = ShowAttachmentDlg(nullptr, hParent, jiraKey, title, fakeAtts);

    std::vector<std::wstring> result;
    for (const auto& att : selected)
        result.push_back(att.contentUrl);
    return result;
}
