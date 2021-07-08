#pragma once
#include <Windows.h>

enum GameGridMode : UINT {
	None,			// Без сетки
	Gap,			// Щель между ячейками
	Grid			// Обычная сетка
};

struct POINTF {
	float x;
	float y;
};

struct THREADDATA
{
	struct THREAD
	{
		int    nDelay;				// ms, если <= 0, то выход
		BOOL   bPause;
		HANDLE hThread;
		DWORD  dwThreadID;

		CRITICAL_SECTION critsection;
	} thread;

	struct BRUSHES
	{
		HBRUSH bkg;
		HBRUSH cell;
	} brush;

	int  nCellSize;
	UINT uCountRow;
	UINT uCountColumn;
	UINT uGridMode;

	struct BUFFER
	{
		HWND hWnd;
		HDC  hDC;
		RECT rc;
	} p;

	BOOL* map;
};

class CGame
{
public:
	CGame();
	~CGame();

	BOOL UpdateBuffer();
	
	BOOL Init(HWND hWndParent, int sz, UINT cols, UINT rows);

	void Pause(BOOL bPause = TRUE);
	void Stop();

	BOOL IsPause();

	int  GetDelay();
	UINT GetGridMode();
	int  GetCellSize();

	void CalcBufferRect(LPRECT lprc);
	void GetBufferRect(LPRECT lprc);

	void SetGridMode(UINT uGridMode);
	void SetOffset(int dx, int dy);
	void SetZoom(int nCellSize, int x = 0, int y = 0);
	void SetDelay(int nDelay);

	void Draw(HWND hWnd, HDC hDC);

	void InvertState(UINT uPos);
	UINT Cursor2Pos(LPPOINT lppt);

	// Случайно заполнить поле
	void Random(float p = 0.3f);

	BOOL Save();

private:
	THREADDATA m_data;
	POINTF m_ptOffset;

	BOOL CreateBuffer();
	void ReleaseBuffer();

	BOOL CheckPause();
	void Update(BOOL bErase);
};

