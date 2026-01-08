#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

#define ID_TRAY_ICON 1001
#define ID_MENU_EXIT 2001
#define ID_MENU_SETTINGS 2002
#define TIMER_IDLE 3001
#define TIMER_HIDE 3002
#define IDLE_TIMEOUT 1000
#define HIDE_TIMEOUT 1000
#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 30
#define BUTTON_RADIUS 8

typedef struct {
    int leftModifiers;
    int leftKey;
    int rightModifiers;
    int rightKey;
    float idleTimeout;
    float hideTimeout;
} Config;

Config g_config = {0, VK_RETURN, 0, VK_ESCAPE, 1.5f, 1.5f};
HHOOK g_mouseHook = NULL;
HWND g_hwnd = NULL;
HWND g_hPopup = NULL;
POINT g_lastPos = {0, 0};
DWORD g_lastMoveTime = 0;
bool g_showPopup = false;
bool g_popupMoved = false;
NOTIFYICONDATAA g_nid = {0};

void LoadConfig() {
    FILE* f = fopen("config.ini", "r");
    if (f) {
        fscanf(f, "leftModifiers=%d\n", &g_config.leftModifiers);
        fscanf(f, "leftKey=%d\n", &g_config.leftKey);
        fscanf(f, "rightModifiers=%d\n", &g_config.rightModifiers);
        fscanf(f, "rightKey=%d\n", &g_config.rightKey);
        fscanf(f, "idleTimeout=%f\n", &g_config.idleTimeout);
        fscanf(f, "hideTimeout=%f\n", &g_config.hideTimeout);
        fclose(f);
    }
}

void SaveConfig() {
    FILE* f = fopen("config.ini", "w");
    if (f) {
        fprintf(f, "leftModifiers=%d\n", g_config.leftModifiers);
        fprintf(f, "leftKey=%d\n", g_config.leftKey);
        fprintf(f, "rightModifiers=%d\n", g_config.rightModifiers);
        fprintf(f, "rightKey=%d\n", g_config.rightKey);
        fprintf(f, "idleTimeout=%.1f\n", g_config.idleTimeout);
        fprintf(f, "hideTimeout=%.1f\n", g_config.hideTimeout);
        fclose(f);
    }
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_MOUSEMOVE) {
            POINT currentPos = pMouse->pt;
            
            if (currentPos.x != g_lastPos.x || currentPos.y != g_lastPos.y) {
                g_lastPos = currentPos;
                g_lastMoveTime = GetTickCount();
                
                if (g_showPopup) {
                    g_popupMoved = true;
                }
            }
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

int GetVkCodeFromName(const char* name) {
    if (strlen(name) == 1) {
        char c = name[0];
        if (c >= 'A' && c <= 'Z') return 'A' + (c - 'A');
        if (c >= 'a' && c <= 'z') return 'A' + (c - 'a');
        if (c >= '0' && c <= '9') return '0' + (c - '0');
    }
    
    if (_strcmpi(name, "Enter") == 0) return VK_RETURN;
    if (_strcmpi(name, "Escape") == 0) return VK_ESCAPE;
    if (_strcmpi(name, "Space") == 0) return VK_SPACE;
    if (_strcmpi(name, "Tab") == 0) return VK_TAB;
    if (_strcmpi(name, "Backspace") == 0) return VK_BACK;
    if (_strcmpi(name, "Delete") == 0) return VK_DELETE;
    if (_strcmpi(name, "Insert") == 0) return VK_INSERT;
    if (_strcmpi(name, "Home") == 0) return VK_HOME;
    if (_strcmpi(name, "End") == 0) return VK_END;
    if (_strcmpi(name, "Page Up") == 0) return VK_PRIOR;
    if (_strcmpi(name, "Page Down") == 0) return VK_NEXT;
    if (_strcmpi(name, "Up") == 0) return VK_UP;
    if (_strcmpi(name, "Down") == 0) return VK_DOWN;
    if (_strcmpi(name, "Left") == 0) return VK_LEFT;
    if (_strcmpi(name, "Right") == 0) return VK_RIGHT;
    if (_strcmpi(name, "F1") == 0) return VK_F1;
    if (_strcmpi(name, "F2") == 0) return VK_F2;
    if (_strcmpi(name, "F3") == 0) return VK_F3;
    if (_strcmpi(name, "F4") == 0) return VK_F4;
    if (_strcmpi(name, "F5") == 0) return VK_F5;
    if (_strcmpi(name, "F6") == 0) return VK_F6;
    if (_strcmpi(name, "F7") == 0) return VK_F7;
    if (_strcmpi(name, "F8") == 0) return VK_F8;
    if (_strcmpi(name, "F9") == 0) return VK_F9;
    if (_strcmpi(name, "F10") == 0) return VK_F10;
    if (_strcmpi(name, "F11") == 0) return VK_F11;
    if (_strcmpi(name, "F12") == 0) return VK_F12;
    
    return 0;
}

const char* GetKeyNameFromVkCode(int vkCode) {
    static char keyName[32];
    
    if (vkCode >= 'A' && vkCode <= 'Z') {
        keyName[0] = (char)vkCode;
        keyName[1] = '\0';
        return keyName;
    }
    
    if (vkCode >= '0' && vkCode <= '9') {
        keyName[0] = (char)vkCode;
        keyName[1] = '\0';
        return keyName;
    }
    
    switch (vkCode) {
        case VK_RETURN: return "Enter";
        case VK_ESCAPE: return "Escape";
        case VK_SPACE: return "Space";
        case VK_TAB: return "Tab";
        case VK_BACK: return "Backspace";
        case VK_DELETE: return "Delete";
        case VK_INSERT: return "Insert";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "Page Up";
        case VK_NEXT: return "Page Down";
        case VK_UP: return "Up";
        case VK_DOWN: return "Down";
        case VK_LEFT: return "Left";
        case VK_RIGHT: return "Right";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        default: return "";
    }
}

void SendKey(int modifiers, int vkCode) {
    if (modifiers & MOD_CONTROL) {
        keybd_event(VK_CONTROL, 0, 0, 0);
    }
    if (modifiers & MOD_ALT) {
        keybd_event(VK_MENU, 0, 0, 0);
    }
    if (modifiers & MOD_SHIFT) {
        keybd_event(VK_SHIFT, 0, 0, 0);
    }
    if (modifiers & MOD_WIN) {
        keybd_event(VK_LWIN, 0, 0, 0);
    }
    
    keybd_event(vkCode, 0, 0, 0);
    keybd_event(vkCode, 0, KEYEVENTF_KEYUP, 0);
    
    if (modifiers & MOD_WIN) {
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    }
    if (modifiers & MOD_SHIFT) {
        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
    }
    if (modifiers & MOD_ALT) {
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    }
    if (modifiers & MOD_CONTROL) {
        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    }
}

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont = NULL;
    
    switch (msg) {
        case WM_CREATE: {
            hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
            break;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
            
            PatBlt(hdcMem, 0, 0, rect.right, rect.bottom, WHITENESS);
            
            HRGN hRgn = CreateRoundRectRgn(0, 0, rect.right, rect.bottom, BUTTON_RADIUS, BUTTON_RADIUS);
            SetWindowRgn(hwnd, hRgn, TRUE);
            
            RECT leftRect = {0, 0, BUTTON_WIDTH, BUTTON_HEIGHT};
            RECT rightRect = {BUTTON_WIDTH + 10, 0, BUTTON_WIDTH * 2 + 10, BUTTON_HEIGHT};
            
            HBRUSH hBrush = CreateSolidBrush(RGB(70, 130, 180));
            FillRect(hdcMem, &leftRect, hBrush);
            FillRect(hdcMem, &rightRect, hBrush);
            DeleteObject(hBrush);
            
            SetBkMode(hdcMem, TRANSPARENT);
            SetTextColor(hdcMem, RGB(255, 255, 255));
            SelectObject(hdcMem, hFont);
            
            char leftText[32];
            char rightText[32];
            GetKeyNameTextA(MapVirtualKeyA(g_config.leftKey, 0) << 16, leftText, 32);
            GetKeyNameTextA(MapVirtualKeyA(g_config.rightKey, 0) << 16, rightText, 32);
            
            DrawTextA(hdcMem, leftText, -1, &leftRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DrawTextA(hdcMem, rightText, -1, &rightRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
            
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            
            EndPaint(hwnd, &ps);
            break;
        }
        
        case WM_LBUTTONDOWN: {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            
            if (pt.x < BUTTON_WIDTH) {
                SendKey(g_config.leftModifiers, g_config.leftKey);
            } else if (pt.x > BUTTON_WIDTH + 10) {
                SendKey(g_config.rightModifiers, g_config.rightKey);
            }
            
            ShowWindow(hwnd, SW_HIDE);
            g_showPopup = false;
            break;
        }
        
        case WM_DESTROY:
            if (hFont) DeleteObject(hFont);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

void CreatePopupWindow() {
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = PopupWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "PopupWindow";
    
    RegisterClassExA(&wc);
    
    g_hPopup = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        "PopupWindow", "", WS_POPUP, 0, 0, BUTTON_WIDTH * 2 + 10, BUTTON_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    
    SetLayeredWindowAttributes(g_hPopup, 0, 200, LWA_ALPHA);
}

void ShowPopupAtCursor() {
    if (!g_hPopup) return;
    
    POINT pt;
    GetCursorPos(&pt);
    
    int x = pt.x - BUTTON_WIDTH - 5;
    int y = pt.y + 20;
    
    SetWindowPos(g_hPopup, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    InvalidateRect(g_hPopup, NULL, TRUE);
    g_showPopup = true;
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hLeftCombo, hRightCombo, hIdleEdit, hHideEdit;
    static HWND hLeftCtrl, hLeftAlt, hLeftShift, hLeftWin;
    static HWND hRightCtrl, hRightAlt, hRightShift, hRightWin;
    
    switch (msg) {
        case WM_CREATE: {
            CreateWindowA("STATIC", "Left Button:", WS_VISIBLE | WS_CHILD,
                10, 10, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindowA("STATIC", "Right Button:", WS_VISIBLE | WS_CHILD,
                10, 90, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindowA("STATIC", "Idle Timeout (s):", WS_VISIBLE | WS_CHILD,
                10, 170, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindowA("STATIC", "Hide Timeout (s):", WS_VISIBLE | WS_CHILD,
                10, 200, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            
            hLeftCombo = CreateWindowA("COMBOBOX", "", CBS_DROPDOWN | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
                120, 10, 150, 200, hwnd, (HMENU)3001, GetModuleHandle(NULL), NULL);
            hRightCombo = CreateWindowA("COMBOBOX", "", CBS_DROPDOWN | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
                120, 90, 150, 200, hwnd, (HMENU)3002, GetModuleHandle(NULL), NULL);
            hIdleEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                120, 170, 150, 20, hwnd, (HMENU)3003, GetModuleHandle(NULL), NULL);
            hHideEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                120, 200, 150, 20, hwnd, (HMENU)3004, GetModuleHandle(NULL), NULL);
            
            hLeftCtrl = CreateWindowA("BUTTON", "Ctrl", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                120, 40, 50, 20, hwnd, (HMENU)3101, GetModuleHandle(NULL), NULL);
            hLeftAlt = CreateWindowA("BUTTON", "Alt", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                175, 40, 50, 20, hwnd, (HMENU)3102, GetModuleHandle(NULL), NULL);
            hLeftShift = CreateWindowA("BUTTON", "Shift", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                230, 40, 50, 20, hwnd, (HMENU)3103, GetModuleHandle(NULL), NULL);
            hLeftWin = CreateWindowA("BUTTON", "Win", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                120, 65, 50, 20, hwnd, (HMENU)3104, GetModuleHandle(NULL), NULL);
            
            hRightCtrl = CreateWindowA("BUTTON", "Ctrl", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                120, 120, 50, 20, hwnd, (HMENU)3201, GetModuleHandle(NULL), NULL);
            hRightAlt = CreateWindowA("BUTTON", "Alt", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                175, 120, 50, 20, hwnd, (HMENU)3202, GetModuleHandle(NULL), NULL);
            hRightShift = CreateWindowA("BUTTON", "Shift", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                230, 120, 50, 20, hwnd, (HMENU)3203, GetModuleHandle(NULL), NULL);
            hRightWin = CreateWindowA("BUTTON", "Win", BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                120, 145, 50, 20, hwnd, (HMENU)3204, GetModuleHandle(NULL), NULL);
            
            const char* keys[] = {"Enter", "Escape", "Space", "Tab", "Backspace", "Delete", "Insert", "Home", "End", "Page Up", "Page Down",
                                "Up", "Down", "Left", "Right", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"};
            
            for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
                SendMessageA(hLeftCombo, CB_ADDSTRING, 0, (LPARAM)keys[i]);
                SendMessageA(hRightCombo, CB_ADDSTRING, 0, (LPARAM)keys[i]);
            }
            
            for (char c = 'A'; c <= 'Z'; c++) {
                char keyName[2] = {c, '\0'};
                SendMessageA(hLeftCombo, CB_ADDSTRING, 0, (LPARAM)keyName);
                SendMessageA(hRightCombo, CB_ADDSTRING, 0, (LPARAM)keyName);
            }
            
            for (char c = '0'; c <= '9'; c++) {
                char keyName[2] = {c, '\0'};
                SendMessageA(hLeftCombo, CB_ADDSTRING, 0, (LPARAM)keyName);
                SendMessageA(hRightCombo, CB_ADDSTRING, 0, (LPARAM)keyName);
            }
            
            CreateWindowA("BUTTON", "OK", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                50, 240, 80, 30, hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            CreateWindowA("BUTTON", "Cancel", WS_VISIBLE | WS_CHILD,
                150, 240, 80, 30, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            
            break;
        }
        
        case WM_INITDIALOG: {
            const char* leftKeyName = GetKeyNameFromVkCode(g_config.leftKey);
            const char* rightKeyName = GetKeyNameFromVkCode(g_config.rightKey);
            SetWindowTextA(hLeftCombo, leftKeyName);
            SetWindowTextA(hRightCombo, rightKeyName);
            
            SendMessageA(hLeftCtrl, BM_SETCHECK, (g_config.leftModifiers & MOD_CONTROL) ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hLeftAlt, BM_SETCHECK, (g_config.leftModifiers & MOD_ALT) ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hLeftShift, BM_SETCHECK, (g_config.leftModifiers & MOD_SHIFT) ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hLeftWin, BM_SETCHECK, (g_config.leftModifiers & MOD_WIN) ? BST_CHECKED : BST_UNCHECKED, 0);
            
            SendMessageA(hRightCtrl, BM_SETCHECK, (g_config.rightModifiers & MOD_CONTROL) ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hRightAlt, BM_SETCHECK, (g_config.rightModifiers & MOD_ALT) ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hRightShift, BM_SETCHECK, (g_config.rightModifiers & MOD_SHIFT) ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hRightWin, BM_SETCHECK, (g_config.rightModifiers & MOD_WIN) ? BST_CHECKED : BST_UNCHECKED, 0);
            
            char idleText[32], hideText[32];
            sprintf_s(idleText, sizeof(idleText), "%.1f", g_config.idleTimeout);
            sprintf_s(hideText, sizeof(hideText), "%.1f", g_config.hideTimeout);
            SetWindowTextA(hIdleEdit, idleText);
            SetWindowTextA(hHideEdit, hideText);
            break;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK) {
                char leftKeyText[32], rightKeyText[32];
                GetWindowTextA(hLeftCombo, leftKeyText, 32);
                GetWindowTextA(hRightCombo, rightKeyText, 32);
                
                g_config.leftKey = GetVkCodeFromName(leftKeyText);
                g_config.rightKey = GetVkCodeFromName(rightKeyText);
                
                if (g_config.leftKey == 0) g_config.leftKey = VK_RETURN;
                if (g_config.rightKey == 0) g_config.rightKey = VK_ESCAPE;
                
                g_config.leftModifiers = 0;
                if (SendMessageA(hLeftCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.leftModifiers |= MOD_CONTROL;
                if (SendMessageA(hLeftAlt, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.leftModifiers |= MOD_ALT;
                if (SendMessageA(hLeftShift, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.leftModifiers |= MOD_SHIFT;
                if (SendMessageA(hLeftWin, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.leftModifiers |= MOD_WIN;
                
                g_config.rightModifiers = 0;
                if (SendMessageA(hRightCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.rightModifiers |= MOD_CONTROL;
                if (SendMessageA(hRightAlt, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.rightModifiers |= MOD_ALT;
                if (SendMessageA(hRightShift, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.rightModifiers |= MOD_SHIFT;
                if (SendMessageA(hRightWin, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.rightModifiers |= MOD_WIN;
                
                char idleText[32], hideText[32];
                GetWindowTextA(hIdleEdit, idleText, 32);
                GetWindowTextA(hHideEdit, hideText, 32);
                g_config.idleTimeout = (float)atof(idleText);
                g_config.hideTimeout = (float)atof(hideText);
                
                if (g_config.idleTimeout < 0.1f) g_config.idleTimeout = 0.1f;
                if (g_config.hideTimeout < 0.1f) g_config.hideTimeout = 0.1f;
                
                SaveConfig();
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == IDCANCEL) {
                DestroyWindow(hwnd);
            }
            break;
        }
        
        case WM_DESTROY:
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

void ShowSettingsDialog() {
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "SettingsWindow";
    
    RegisterClassExA(&wc);
    
    HWND hSettings = CreateWindowExA(0, "SettingsWindow", "Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 280, 300,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    
    ShowWindow(hSettings, SW_SHOW);
    UpdateWindow(hSettings);
}

HICON LoadIconFile(const char* filename) {
    char fullPath[MAX_PATH];
    HICON hIcon = NULL;
    
    GetModuleFileNameA(NULL, fullPath, MAX_PATH);
    char* lastSlash = strrchr(fullPath, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
        strcat_s(fullPath, MAX_PATH, filename);
    } else {
        strcpy_s(fullPath, MAX_PATH, filename);
    }
    
    hIcon = (HICON)LoadImageA(NULL, fullPath, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    if (!hIcon) {
        hIcon = (HICON)LoadImageA(NULL, fullPath, IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    }
    if (!hIcon) {
        hIcon = (HICON)LoadImageA(NULL, fullPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
    if (!hIcon) {
        UINT iconCount = ExtractIconExA(fullPath, 0, NULL, &hIcon, 1);
        if (iconCount == 0) {
            hIcon = NULL;
        }
    }
    
    return hIcon;
}

void AddTrayIcon() {
    HICON hIcon = LoadIconFile("favicon.ico");
    
    g_nid.cbSize = sizeof(NOTIFYICONDATAA);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = ID_TRAY_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_USER + 1;
    g_nid.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
    strcpy_s(g_nid.szTip, sizeof(g_nid.szTip), "Mouse Enhancer");
    
    Shell_NotifyIconA(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconA(NIM_DELETE, &g_nid);
}

void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    AppendMenuA(hMenu, MF_STRING, ID_MENU_SETTINGS, "Settings");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_MENU_EXIT, "Exit");
    
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_mouseHook = SetWindowsHookExA(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
            SetTimer(hwnd, TIMER_IDLE, 100, NULL);
            break;
            
        case WM_TIMER: {
            if (wParam == TIMER_IDLE) {
                DWORD currentTime = GetTickCount();
                DWORD timeSinceLastMove = currentTime - g_lastMoveTime;
                DWORD idleTimeoutMs = (DWORD)(g_config.idleTimeout * 1000);
                DWORD hideTimeoutMs = (DWORD)(g_config.hideTimeout * 1000);
                
                if (!g_showPopup && timeSinceLastMove >= idleTimeoutMs) {
                    ShowPopupAtCursor();
                    g_popupMoved = false;
                } else if (g_showPopup && g_popupMoved && timeSinceLastMove >= hideTimeoutMs) {
                    ShowWindow(g_hPopup, SW_HIDE);
                    g_showPopup = false;
                    g_popupMoved = false;
                }
            }
            break;
        }
        
        case WM_USER + 1: {
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ShowSettingsDialog();
            }
            break;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_MENU_EXIT) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            } else if (LOWORD(wParam) == ID_MENU_SETTINGS) {
                ShowSettingsDialog();
            }
            break;
        }
        
        case WM_CLOSE:
            RemoveTrayIcon();
            if (g_mouseHook) UnhookWindowsHookEx(g_mouseHook);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LoadConfig();
    CreatePopupWindow();
    
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "MainWindow";
    
    RegisterClassExA(&wc);
    
    g_hwnd = CreateWindowExA(0, "MainWindow", "Mouse Enhancer",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL);
    
    ShowWindow(g_hwnd, SW_HIDE);
    UpdateWindow(g_hwnd);
    
    AddTrayIcon();
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
