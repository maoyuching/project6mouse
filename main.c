#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

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
    int leftKey;
    int rightKey;
    float idleTimeout;
    float hideTimeout;
} Config;

Config g_config = {VK_RETURN, VK_ESCAPE, 1.5f, 1.5f};
HHOOK g_mouseHook = NULL;
HWND g_hwnd = NULL;
HWND g_hPopup = NULL;
POINT g_lastPos = {0, 0};
DWORD g_lastMoveTime = 0;
bool g_showPopup = false;
bool g_popupMoved = false;
NOTIFYICONDATAA g_nid = {0};
ULONG_PTR g_gdiplusToken = 0;

void LoadConfig() {
    FILE* f = fopen("config.ini", "r");
    if (f) {
        fscanf(f, "leftKey=%d\n", &g_config.leftKey);
        fscanf(f, "rightKey=%d\n", &g_config.rightKey);
        fscanf(f, "idleTimeout=%f\n", &g_config.idleTimeout);
        fscanf(f, "hideTimeout=%f\n", &g_config.hideTimeout);
        fclose(f);
    }
}

void SaveConfig() {
    FILE* f = fopen("config.ini", "w");
    if (f) {
        fprintf(f, "leftKey=%d\n", g_config.leftKey);
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

void SendKey(int vkCode) {
    keybd_event(vkCode, 0, 0, 0);
    keybd_event(vkCode, 0, KEYEVENTF_KEYUP, 0);
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
                SendKey(g_config.leftKey);
            } else if (pt.x > BUTTON_WIDTH + 10) {
                SendKey(g_config.rightKey);
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
    
    switch (msg) {
        case WM_CREATE: {
            CreateWindowA("STATIC", "Left Button:", WS_VISIBLE | WS_CHILD,
                10, 10, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindowA("STATIC", "Right Button:", WS_VISIBLE | WS_CHILD,
                10, 40, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindowA("STATIC", "Idle Timeout (s):", WS_VISIBLE | WS_CHILD,
                10, 70, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            CreateWindowA("STATIC", "Hide Timeout (s):", WS_VISIBLE | WS_CHILD,
                10, 100, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            
            hLeftCombo = CreateWindowA("COMBOBOX", "", CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
                120, 10, 150, 200, hwnd, (HMENU)3001, GetModuleHandle(NULL), NULL);
            hRightCombo = CreateWindowA("COMBOBOX", "", CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
                120, 40, 150, 200, hwnd, (HMENU)3002, GetModuleHandle(NULL), NULL);
            hIdleEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                120, 70, 150, 20, hwnd, (HMENU)3003, GetModuleHandle(NULL), NULL);
            hHideEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                120, 100, 150, 20, hwnd, (HMENU)3004, GetModuleHandle(NULL), NULL);
            
            const char* keys[] = {"Enter", "Escape", "Space", "Tab", "Backspace", "Delete", "Insert", "Home", "End", "Page Up", "Page Down"};
            int keyCodes[] = {VK_RETURN, VK_ESCAPE, VK_SPACE, VK_TAB, VK_BACK, VK_DELETE, VK_INSERT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT};
            
            for (int i = 0; i < 11; i++) {
                SendMessageA(hLeftCombo, CB_ADDSTRING, 0, (LPARAM)keys[i]);
                SendMessageA(hRightCombo, CB_ADDSTRING, 0, (LPARAM)keys[i]);
            }
            
            CreateWindowA("BUTTON", "OK", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                50, 140, 80, 30, hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            CreateWindowA("BUTTON", "Cancel", WS_VISIBLE | WS_CHILD,
                150, 140, 80, 30, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            
            break;
        }
        
        case WM_INITDIALOG: {
            int leftIndex = 0, rightIndex = 1;
            SendMessageA(hLeftCombo, CB_SETCURSEL, leftIndex, 0);
            SendMessageA(hRightCombo, CB_SETCURSEL, rightIndex, 0);
            
            char idleText[32], hideText[32];
            sprintf_s(idleText, sizeof(idleText), "%.1f", g_config.idleTimeout);
            sprintf_s(hideText, sizeof(hideText), "%.1f", g_config.hideTimeout);
            SetWindowTextA(hIdleEdit, idleText);
            SetWindowTextA(hHideEdit, hideText);
            break;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK) {
                int leftIndex = SendMessageA(hLeftCombo, CB_GETCURSEL, 0, 0);
                int rightIndex = SendMessageA(hRightCombo, CB_GETCURSEL, 0, 0);
                
                int keyCodes[] = {VK_RETURN, VK_ESCAPE, VK_SPACE, VK_TAB, VK_BACK, VK_DELETE, VK_INSERT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT};
                g_config.leftKey = keyCodes[leftIndex];
                g_config.rightKey = keyCodes[rightIndex];
                
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
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 280, 200,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    
    ShowWindow(hSettings, SW_SHOW);
    UpdateWindow(hSettings);
}

HICON LoadPngIcon(const char* filename) {
    GpBitmap* bitmap = NULL;
    GpStatus status;
    HICON hIcon = NULL;
    char fullPath[MAX_PATH];
    WCHAR wFilename[MAX_PATH];
    
    GetModuleFileNameA(NULL, fullPath, MAX_PATH);
    char* lastSlash = strrchr(fullPath, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
        strcat_s(fullPath, MAX_PATH, filename);
    } else {
        strcpy_s(fullPath, MAX_PATH, filename);
    }
    
    MultiByteToWideChar(CP_ACP, 0, fullPath, -1, wFilename, MAX_PATH);
    
    status = GdipCreateBitmapFromFile(wFilename, &bitmap);
    if (status != Ok || !bitmap) {
        return NULL;
    }
    
    status = GdipCreateHICONFromBitmap(bitmap, &hIcon);
    if (status != Ok) {
        hIcon = NULL;
    }
    
    GdipDisposeImage((GpImage*)bitmap);
    return hIcon;
}

void AddTrayIcon() {
    HICON hIcon = LoadPngIcon("icons8.png");
    
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
            if (g_gdiplusToken) GdiplusShutdown(g_gdiplusToken);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    
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
