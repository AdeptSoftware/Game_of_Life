#include "resource.h"
#include "CGameOption.h"
#pragma warning(disable:6054)

// Константы отображения элементов управления (ЭУ)
const int g_gap	= 3;	// Зазор между ЭУ
const int g_w	= 50;
const int g_h	= 20;
const int g_x	= 5;	// Расположение первого ЭУ по оси X
const int g_y	= 5;	// Расположение первого ЭУ по оси Y

HFONT MakeFont(LPCWSTR szTypeface, int nSize, BOOL bBold, BOOL bItalic) {
	HDC hDC = GetDC(nullptr);
	if (hDC) {
		// Узнаем, сколько пикселей в одном логическом дюйме
		int pixelsPerInch = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);
		ReleaseDC(nullptr, hDC);
		// Узнаем высоту в пикселях шрифта размером Size пунктов
		int nFontHeight = -MulDiv(nSize, pixelsPerInch, 72);
		return CreateFont(nFontHeight, 0, 0, 0, (bBold ? FW_BOLD : FW_NORMAL), bItalic, 0, 0,
						  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
						  DEFAULT_PITCH | FF_DONTCARE, szTypeface);
	}
	return nullptr;
}

CGameOption::CGameOption() {
	m_hEditWndWidth  = nullptr;
	m_hEditWndHeight = nullptr;
	m_hEditCellSize  = nullptr;
	m_hButtonStart	 = nullptr;
	m_hFont = MakeFont(L"Consolas", 10, FALSE, FALSE);
}

CGameOption::~CGameOption() {
	if(m_hFont)
		DeleteObject(m_hFont);
}

// Установка размеров окна по размеру всех ЭУ
void CGameOption::UpdateWindowPos(HWND hWndParent) {
	RECT rcDesktop, rcTemp{0};
	// Размер c границами, заголовком и прочим
	AdjustWindowRect(&rcTemp, GetWindowLongPtr(hWndParent, GWL_STYLE), FALSE);

	int w = 2*g_x+3*g_gap+5*g_w+(rcTemp.right-rcTemp.left);
	int h = 2*g_y+g_h+(rcTemp.bottom-rcTemp.top);
	GetClientRect(GetDesktopWindow(), &rcDesktop);
	SetWindowPos(hWndParent, nullptr, (rcDesktop.right-w)/2, (rcDesktop.bottom-h)/2, w, h, SWP_NOZORDER);
}

BOOL CGameOption::CreateControls(HINSTANCE hInst, HWND hWndParent) {
	DWORD dwStyle = ES_CENTER | ES_NUMBER | WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE;
	m_hEditWndWidth  = CreateWindowW(L"Edit", L"50", dwStyle, g_x,				 g_y, g_w,   g_h, hWndParent, nullptr, hInst, nullptr);
	m_hEditWndHeight = CreateWindowW(L"Edit", L"50", dwStyle, g_x+g_w+g_gap,	 g_y, g_w,   g_h, hWndParent, nullptr, hInst, nullptr);
	m_hEditCellSize  = CreateWindowW(L"Edit", L"5",  dwStyle, g_x+2*(g_w+g_gap), g_y, g_w,   g_h, hWndParent, nullptr, hInst, nullptr);

	HMENU hCtrlID = reinterpret_cast<HMENU>(IDC_BUTTON_START);
	dwStyle = BS_CENTER | BS_PUSHBUTTON | WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE;
	m_hButtonStart = CreateWindowW(L"Button", L"OK", dwStyle, g_x+3*(g_w+g_gap), g_y, g_w*2, g_h, hWndParent, hCtrlID, hInst, nullptr);
	if (m_hButtonStart && m_hEditCellSize && m_hEditWndHeight && m_hEditWndWidth) {
		WPARAM wp = reinterpret_cast<WPARAM>(m_hFont);
		SendMessageW(m_hEditWndHeight, WM_SETFONT, wp, FALSE);
		SendMessageW(m_hEditWndWidth,  WM_SETFONT, wp, FALSE);
		SendMessageW(m_hEditCellSize,  WM_SETFONT, wp, FALSE);
		SendMessageW(m_hButtonStart,   WM_SETFONT, wp, FALSE);
		return TRUE;
	}
	return FALSE;
}

void CGameOption::Show(int nCmdShow) {
	ShowWindow(m_hEditWndHeight, nCmdShow);
	ShowWindow(m_hEditWndWidth,  nCmdShow);
	ShowWindow(m_hEditCellSize,  nCmdShow);
	ShowWindow(m_hButtonStart,   nCmdShow);
}

BOOL CGameOption::IsHide() {
	return !IsWindowVisible(m_hButtonStart);
}

int GetNumber(HWND hEdit) {
	WCHAR szText[16];
	GetWindowTextW(hEdit, szText, 16);
	return _wtoi(szText);
}

int CGameOption::GetWindowWidth() {
	return GetNumber(m_hEditWndWidth);
}

int CGameOption::GetWindowHeight() {
	return GetNumber(m_hEditWndHeight);
}

int CGameOption::GetCellSize() {
	return GetNumber(m_hEditCellSize);
}
