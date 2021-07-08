#include "resource.h"
#include "CGameOption.h"
#pragma warning(disable:6054)

#define X			 5
#define Y			 5
#define SPACE		 3	
#define W			50
#define H			20

HFONT MakeFont(LPCWSTR Typeface, int nSize, BOOL bBold, BOOL bItalic) {
	// Узнаем, сколько пикселей в одном логическом дюйме
	int pixelsPerInch = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);
	// Узнаем высоту в пикселях шрифта размером Size пунктов
	int fontHeight = -MulDiv(nSize, pixelsPerInch, 72);
	return CreateFont(fontHeight, 0, 0, 0, (bBold ? FW_BOLD : FW_NORMAL), bItalic, 0, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, Typeface);
}

CGameOption::CGameOption() {
	m_hEditWndWidth  = NULL;
	m_hEditWndHeight = NULL;
	m_hEditCellSize  = NULL;
	m_hButtonNext	 = NULL;
	m_hFont = MakeFont(L"Consolas", 10, FALSE, FALSE);
}

CGameOption::~CGameOption() {
	if(m_hFont)
		DeleteObject(m_hFont);
}

void CGameOption::UpdateWindowPos(HWND hWndParent) {
	RECT rcDesktop;
	int w = 2*X+3*SPACE+5*W+15;
	int h = 2*Y+H+36;
	GetClientRect(GetDesktopWindow(), &rcDesktop);
	SetWindowPos(hWndParent, NULL, (rcDesktop.right-w)/2, (rcDesktop.bottom-h)/2, w, h, SWP_NOZORDER);
}

BOOL CGameOption::CreateControls(HINSTANCE hInst, HWND hWndParent) {
	DWORD dwStyle = ES_CENTER | ES_NUMBER | WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE;
	m_hEditWndWidth  = CreateWindowW(L"Edit", L"675", dwStyle, X, Y, W, H, hWndParent, NULL, hInst, NULL);
	m_hEditWndHeight = CreateWindowW(L"Edit", L"360", dwStyle, X+W+SPACE, Y, W, H, hWndParent, NULL, hInst, NULL);
	m_hEditCellSize  = CreateWindowW(L"Edit", L"1",  dwStyle, X + 2 * (W + SPACE), Y, W, H, hWndParent, NULL, hInst, NULL);

	dwStyle = BS_CENTER | BS_PUSHBUTTON | WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE;
	m_hButtonNext = CreateWindowW(L"Button", L"Next", dwStyle, X+3*(W+SPACE), Y, W*2, H, hWndParent, (HMENU)IDC_BUTTON_NEXT, hInst, NULL);
	if (m_hButtonNext && m_hEditCellSize && m_hEditWndHeight && m_hEditWndWidth) {
		SendMessageW(m_hEditWndWidth, WM_SETFONT, (WPARAM)m_hFont, FALSE);
		SendMessageW(m_hEditWndHeight, WM_SETFONT, (WPARAM)m_hFont, FALSE);
		SendMessageW(m_hEditCellSize, WM_SETFONT, (WPARAM)m_hFont, FALSE);
		SendMessageW(m_hButtonNext, WM_SETFONT, (WPARAM)m_hFont, FALSE);
		return TRUE;
	}
	return FALSE;
}

void CGameOption::Show(int nCmdShow) {
	ShowWindow(m_hEditWndWidth, nCmdShow);
	ShowWindow(m_hEditWndHeight, nCmdShow);
	ShowWindow(m_hEditCellSize, nCmdShow);
	ShowWindow(m_hButtonNext, nCmdShow);
}

BOOL CGameOption::IsHide() {
	return !IsWindowVisible(m_hButtonNext);
}

int GetNumber(HWND hEdit) {
	WCHAR szText[16];
	GetWindowTextW(hEdit, szText, 16);
	return _wtoi(szText);
}

UINT CGameOption::GetWindowWidth() {
	return (UINT)GetNumber(m_hEditWndWidth);
}

UINT CGameOption::GetWindowHeight() {
	return (UINT)GetNumber(m_hEditWndHeight);
}

UINT CGameOption::GetCellSize() {
	return (UINT)GetNumber(m_hEditCellSize);
}
