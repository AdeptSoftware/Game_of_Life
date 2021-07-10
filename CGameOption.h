#pragma once
#include <Windows.h>

class CGameOption {
public:
	CGameOption();
	~CGameOption();

	BOOL CreateControls(HINSTANCE hInst, HWND hWndParent);
	void Show(int nCmdShow);
	BOOL IsHide();	// Скрывает/отображает все элементы управления

	void UpdateWindowPos(HWND hWndParent);

	int GetWindowWidth();
	int GetWindowHeight();
	int GetCellSize();

private:
	HWND m_hEditWndWidth;
	HWND m_hEditWndHeight;
	HWND m_hEditCellSize;

	HWND m_hButtonStart;

	HFONT m_hFont;
};

