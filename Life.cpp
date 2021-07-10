#include <SDKDDKVer.h>
#include "MemoryLeaks.h"    // Автоматическое отслеживание утечек памяти при Debug-конфигурации
#include "CGameOption.h"
#include "resource.h"
#include "CGame.h"

#include <windowsx.h>       // Функции: GET_X_LPARAM, GET_Y_LPARAM

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

const UINT  g_uMaxLoadString  = 128;
const int   g_uSpeedStep      = 100;

BOOL        g_bLastPauseState = FALSE;
POINT       g_ptCaptured      = { 0, 0 };
CGameOption g_option;
CGame       g_game;

ATOM MyRegisterClass(HINSTANCE hInstance, WCHAR* szWindowClass) {
    WNDCLASSEXW wcex;
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = nullptr;
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WCHAR szWindowClass[g_uMaxLoadString] = L"wndClsName";
    WCHAR szTitle[g_uMaxLoadString] = L"Game of Life (Made by Adept)";
    // LoadStringW(hInstance, IDS_APP_TITLE, szTitle, g_uMaxLoadString);
    MyRegisterClass(hInst, szWindowClass);
    
    // Инициализация
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, 0, CW_USEDEFAULT, nullptr, nullptr, hInst, nullptr);
    if (!hWnd || !g_option.CreateControls(hInst, hWnd))
        return FALSE;

    g_option.UpdateWindowPos(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_GAME_ACCEL));
    while (GetMessage(&msg, nullptr, 0, 0)) { // Цикл основного сообщения
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

// ========= ========= ========= ========= ========= ========= ========= =========

void OnChangedSize(HWND hWnd, BOOL bCenterWindow) {
    UINT uOffset = 0;
    RECT rcBuffer, rcDesktop;
    g_game.CalcBufferRect(&rcBuffer);
    
    // Ограничим размеры окна, чтобы не вышло за пределы экрана
    GetClientRect(GetDesktopWindow(), &rcDesktop);
    if (rcDesktop.right < rcBuffer.right)
        rcBuffer.right = rcDesktop.right;
    if (rcDesktop.bottom < rcBuffer.bottom-32)
        rcBuffer.bottom = rcDesktop.bottom-32;

    if (bCenterWindow) {// Выставим окно по центру
        int x = (rcDesktop.right-rcBuffer.right-16)/2;
        int y = (rcDesktop.bottom-rcBuffer.bottom-32)/2;
        SetWindowPos(hWnd, nullptr, x, y, rcBuffer.right+14, rcBuffer.bottom+37, SWP_NOZORDER);
    }
    else
        SetWindowPos(hWnd, nullptr, 0, 0, rcBuffer.right+14, rcBuffer.bottom+37, SWP_NOMOVE | SWP_NOZORDER);
}

void OnOK(HWND hWnd) {
    int w = g_option.GetWindowWidth();
    int h = g_option.GetWindowHeight();
    int a = g_option.GetCellSize();
    if (g_game.Init(hWnd, a, w, h)) {
        g_option.Show(SW_HIDE);
        OnChangedSize(hWnd, TRUE);
        g_game.UpdateBuffer();

        RECT rc;
        GetClientRect(GetDesktopWindow(), &rc);
        SetCursorPos((rc.right/2)-2, (rc.bottom/2)+13);
    }
}

// Реализация перетаскивания игрового поля
BOOL OnCapturedMessage(HWND hWnd, UINT uMsg, LPARAM lParam) {
    BOOL bCaptured = (GetCapture() != nullptr);
    if (uMsg == WM_RBUTTONUP && bCaptured)
        ReleaseCapture();
    else if (bCaptured && uMsg == WM_MOUSEMOVE) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        g_game.SetOffset(g_ptCaptured.x-x, g_ptCaptured.y-y);
        g_ptCaptured = { x, y };
    }
    else if (uMsg == WM_RBUTTONDOWN && !bCaptured) {
        g_ptCaptured = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SetCapture(hWnd);
    }
    else
        return FALSE;
    return TRUE;
}

BOOL OnCommandMessage(HWND hWnd, WPARAM wParam) {
    int wmID = LOWORD(wParam);
    if (wmID == IDC_BUTTON_START)
        OnOK(hWnd);
    else if (g_option.IsHide()) {
        if (wmID == ID_GAME_PAUSE)
            g_game.Pause(!g_game.IsPause());
        else if (wmID == ID_GAME_SLOW)
            g_game.SetDelay(g_game.GetDelay()+g_uSpeedStep);
        else if (wmID == ID_GAME_FAST) {
            int nDelay = g_game.GetDelay()-g_uSpeedStep;
            g_game.SetDelay((nDelay > 0) ? nDelay : 1);
        }
        else if (wmID == ID_GAME_GRIDMODE) {
            UINT uGridMode = g_game.GetGridMode()+1;
            if (uGridMode >= GameGridMode::EnumCount)
                uGridMode = 0;
            g_game.SetGridMode(uGridMode);
        }
        else if (wmID == ID_GAME_OFFSET_LEFT)
            g_game.SetOffset(1, 0);
        else if (wmID == ID_GAME_OFFSET_UP)
            g_game.SetOffset(0, 1);
        else if (wmID == ID_GAME_OFFSET_RIGHT)
            g_game.SetOffset(-1, 0);
        else if (wmID == ID_GAME_OFFSET_DOWN)
            g_game.SetOffset(0, -1);
        else if (wmID == ID_GAME_RAND)
            g_game.Random(0.2f);
        else if (wmID == ID_SAVE_BITMAP)
            g_game.Save();
        else
            return FALSE;
    }
    else
        return FALSE;
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_ENTERSIZEMOVE) {
        g_bLastPauseState = g_game.IsPause();
        g_game.Pause(TRUE);
    }
    else if (uMsg == WM_EXITSIZEMOVE) {
        g_game.UpdateBuffer();
        g_game.Pause(g_bLastPauseState);
    }
    else if (uMsg == WM_LBUTTONDOWN && g_option.IsHide()) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        g_game.InvertState(g_game.Cursor2Pos(&pt));
    }
    else if (OnCapturedMessage(hWnd, uMsg, lParam))
        return 0;
    else if (uMsg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        g_game.Draw(hWnd, hDC);
        EndPaint(hWnd, &ps);
    }
    else if (uMsg == WM_COMMAND && OnCommandMessage(hWnd, wParam))
        return 0;
    else if (uMsg == WM_MOUSEWHEEL) {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/120;
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);
        g_game.SetZoom(g_game.GetCellSize()-zDelta, pt.x, pt.y);
    }
    else if (uMsg == WM_DESTROY)
        PostQuitMessage(0);
    else
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    return 0;
}