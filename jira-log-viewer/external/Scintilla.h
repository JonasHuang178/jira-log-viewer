#pragma once

#include <windows.h>

// ---------------------------------------------------------------------------
//  Scintilla — minimal subset needed by Notepad++ plugins
// ---------------------------------------------------------------------------

struct Sci_NotifyHeader {
    HWND     hwndFrom;
    UINT_PTR idFrom;
    unsigned int code;
};

struct SCNotification {
    Sci_NotifyHeader nmhdr;
    int      position;
    int      ch;
    int      modifiers;
    int      modificationType;
    const char* text;
    int      length;
    int      linesAdded;
    int      message;
    WPARAM   wParam;
    LPARAM   lParam;
    int      line;
    int      foldLevelNow;
    int      foldLevelPrev;
    int      margin;
    int      listType;
    int      x;
    int      y;
    int      token;
    int      annotationLinesAdded;
    int      updated;
};

// Scintilla notifications
#define SCN_UPDATEUI 2007
