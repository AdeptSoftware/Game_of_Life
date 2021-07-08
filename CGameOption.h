#pragma once
#include <Windows.h>

class CGameOption {
public:
	CGameOption();
	~CGameOption();

	BOOL CreateControls(HINSTANCE hInst, HWND hWndParent);
	void Show(int nCmdShow);
	BOOL IsHide();

	void UpdateWindowPos(HWND hWndParent);

	UINT GetWindowWidth();
	UINT GetWindowHeight();
	UINT GetCellSize();

private:
	HWND m_hEditWndWidth;
	HWND m_hEditWndHeight;
	HWND m_hEditCellSize;

	HWND m_hButtonNext;

	HFONT m_hFont;
};

