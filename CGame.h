#pragma once
#include <Windows.h>

enum GameGridMode : UINT {
	None,			// Без сетки
	Gap,			// Щель между ячейками
	Grid,			// Обычная сетка
	EnumCount		// Нужен для определения кол-ва доступных вариантов сетки
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
	int  nCountRow;
	int  nCountColumn;
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

	BOOL UpdateBuffer();			// Обновление размер буффера (Bitmap'а)
	
	BOOL Init(HWND hWndParent, int sz, int cols, int rows);

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

	void InvertState(int nPos);		// Изменяет состояние ячейки
	UINT Cursor2Pos(LPPOINT lppt);

	void Random(float p = 0.3f); 	// Случайно заполнить поле

	BOOL Save();					// Сохранение текущего Bitmap'а в файл

private:
	THREADDATA m_data;
	POINTF m_ptOffset;

	BOOL CreateBuffer();			// Создание двойного буфера
	void ReleaseBuffer();

	BOOL CheckPause();
	void Invalidate(BOOL bErase);	// Перерисовка
};

