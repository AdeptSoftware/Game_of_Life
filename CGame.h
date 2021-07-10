#pragma once
#include <Windows.h>

enum GameGridMode : UINT {
	None,			// ��� �����
	Gap,			// ���� ����� ��������
	Grid,			// ������� �����
	EnumCount		// ����� ��� ����������� ���-�� ��������� ��������� �����
};

struct POINTF {
	float x;
	float y;
};

struct THREADDATA
{
	struct THREAD
	{
		int    nDelay;				// ms, ���� <= 0, �� �����
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

	BOOL UpdateBuffer();			// ���������� ������ ������� (Bitmap'�)
	
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

	void InvertState(int nPos);		// �������� ��������� ������
	UINT Cursor2Pos(LPPOINT lppt);

	void Random(float p = 0.3f); 	// �������� ��������� ����

	BOOL Save();					// ���������� �������� Bitmap'� � ����

private:
	THREADDATA m_data;
	POINTF m_ptOffset;

	BOOL CreateBuffer();			// �������� �������� ������
	void ReleaseBuffer();

	BOOL CheckPause();
	void Invalidate(BOOL bErase);	// �����������
};

