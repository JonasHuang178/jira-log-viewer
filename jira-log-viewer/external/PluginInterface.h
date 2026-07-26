#pragma once

#include <windows.h>
#include <tchar.h>

// ---------------------------------------------------------------------------
//  Notepad++ Plugin SDK — minimal subset for plugin development
// ---------------------------------------------------------------------------

const int nbChar = 64;

struct NppData {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
};

typedef void (__cdecl* PFUNCPLUGINCMD)();

struct ShortcutKey {
    bool _isCtrl;
    bool _isAlt;
    bool _isShift;
    UCHAR _key;
};

struct FuncItem {
    TCHAR          _itemName[nbChar];
    PFUNCPLUGINCMD _pFunc;
    int            _cmdID;
    bool           _init2Check;
    ShortcutKey*   _pShKey;
};

// ---------------------------------------------------------------------------
//  Notepad++ messages  (WM_USER + 1000 = NPPMSG)
// ---------------------------------------------------------------------------
#define NPPMSG                     (WM_USER + 1000)

#define NPPM_GETCURRENTSCINTILLA   (NPPMSG + 4)
#define NPPM_GETNPPDIRECTORY       (NPPMSG + 43)
#define NPPM_GETPLUGINSCONFIGDIR   (NPPMSG + 46)
#define NPPM_GETNPPVERSION         (NPPMSG + 50)
#define NPPM_DOOPEN                (NPPMSG + 77)
#define NPPM_GETCURRENTBUFFERID    (NPPMSG + 60)
#define NPPM_SETSTATUSBAR          (NPPMSG + 24)
#define NPPM_GETPLUGINHOMEPATH     (NPPMSG + 97)

#define STATUSBAR_DOC_TYPE   0
#define STATUSBAR_DOC_SIZE   1
#define STATUSBAR_CUR_POS    2
#define STATUSBAR_EOF_FORMAT 3
#define STATUSBAR_UNICODE_TYPE 4
#define STATUSBAR_TYPING_MODE  5

// ---------------------------------------------------------------------------
//  Notepad++ notifications
// ---------------------------------------------------------------------------
#define NPPN_FIRST              1000
#define NPPN_READY              (NPPN_FIRST + 1)
#define NPPN_TBMODIFICATION     (NPPN_FIRST + 2)
#define NPPN_FILEBEFORECLOSE    (NPPN_FIRST + 3)
#define NPPN_FILEOPENED         (NPPN_FIRST + 4)
#define NPPN_FILECLOSED         (NPPN_FIRST + 5)
#define NPPN_BUFFERACTIVATED    (NPPN_FIRST + 9)
#define NPPN_SHUTDOWN           (NPPN_FIRST + 11)
