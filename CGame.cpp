#include "CGame.h"
#include "Debug.h"
#include "BitmapSaver.h"
#include <random>
#define OFFSET				   2
#define GRID_WIDTH			   1

DWORD WINAPI ThreadProc(LPVOID lpParam);
inline void UpdateBitmap(THREADDATA* pData);

void CGame::Update(BOOL bErase) {
	EnterCriticalSection(&m_data.thread.critsection);

	UpdateBitmap(&m_data);
	InvalidateRect(m_data.p.hWnd, nullptr, bErase);

	LeaveCriticalSection(&m_data.thread.critsection);
}

CGame::CGame() {
	m_data.thread.nDelay	 = 0;
	m_data.thread.dwThreadID = 0;
	m_data.thread.hThread	 = NULL;
	m_data.thread.bPause	 = FALSE;

	m_data.uGridMode	     = GameGridMode::None;
	m_data.nCellSize		 = 0;
	m_data.uCountRow		 = 0;
	m_data.uCountColumn		 = 0;

	m_ptOffset		  = { 0, 0 };
	
	m_data.p.hWnd	  = NULL;
	m_data.p.hDC	  = NULL;
	m_data.p.rc		  = { 0, 0, 0, 0 };
	m_data.brush.cell   = CreateSolidBrush(RGB(0, 0, 0));
	m_data.brush.bkg    = CreateSolidBrush(RGB(255, 255, 255));

	InitializeCriticalSection(&m_data.thread.critsection);
}

CGame::~CGame()
{
	Stop();
	ReleaseBuffer();
	DeleteObject(m_data.brush.bkg);
	DeleteObject(m_data.brush.cell);
	DeleteCriticalSection(&m_data.thread.critsection);
}

HDC CreateBuffer(HDC hWindowDC, HFONT hFont, int cx, int cy) {
	HDC hBufferDC = CreateCompatibleDC(hWindowDC);
	HBITMAP hBitmap = (HBITMAP)CreateCompatibleBitmap(hWindowDC, cx, cy);
	DeleteObject(SelectObject(hBufferDC, hBitmap));
	DeleteObject(SelectObject(hBufferDC, hFont));
	return hBufferDC;
}

void DeleteBuffer(HDC hWindowDC, HDC hBufferDC, int x, int y, int cx, int cy) {
	BitBlt(hWindowDC, 0, 0, cx, cy, hBufferDC, x, y, SRCCOPY);
	HBITMAP hBitmap = (HBITMAP)GetCurrentObject(hBufferDC, OBJ_BITMAP);
	DeleteDC(hBufferDC);
	DeleteObject(hBitmap);
}

void CGame::Draw(HWND hWnd, HDC hWindowDC)
{
	if (m_data.p.hDC)
	{
		RECT rcWindow;
		GetClientRect(hWnd, &rcWindow);
		HDC hDC = ::CreateBuffer(hWindowDC, NULL, rcWindow.right, rcWindow.bottom);

		FillRect(hDC, &rcWindow, m_data.brush.bkg);
		BitBlt(hDC, 0, 0, rcWindow.right, rcWindow.bottom, m_data.p.hDC, (int)m_ptOffset.x, (int)m_ptOffset.y, SRCCOPY);

		::DeleteBuffer(hWindowDC, hDC, 0, 0, rcWindow.right, rcWindow.bottom);
	}
}

BOOL CGame::CheckPause() {
	if (!m_data.thread.bPause) {
		MessageBox(NULL, L"Установка доступна только на паузе!", L"Warning!", MB_OK);
		return FALSE;
	}
	return TRUE;
}

void CGame::InvertState(UINT uPos) {
	if (!CheckPause())
		return;
	if (uPos < m_data.uCountColumn*m_data.uCountRow) {
		m_data.map[uPos] = !m_data.map[uPos];
		Update(FALSE);
	}
}

UINT CGame::Cursor2Pos(LPPOINT lppt) {
	//OutputDebug(L"mouse: %i;%i\toffset: %.2f;%.2f\trc: %i;%i\n", lppt->x, lppt->y, m_ptOffset.x, m_ptOffset.y, m_data.p.rc.right, m_data.p.rc.bottom);
	if (!m_data.nCellSize)
		return 0;
	int nGrid = (m_data.uGridMode ? GRID_WIDTH : 0);
	int x = (lppt->x+(int)m_ptOffset.x-OFFSET+nGrid)/(nGrid+m_data.nCellSize);
	int y = (lppt->y+ (int)m_ptOffset.y-OFFSET+nGrid)/(nGrid+m_data.nCellSize);
	if ((x < 0 || x >= (int)m_data.uCountColumn) || (y < 0 || y >= (int)m_data.uCountRow))
		return m_data.uCountColumn*m_data.uCountRow;
	return UINT(y*m_data.uCountColumn+x);
}

void CGame::CalcBufferRect(LPRECT lprc)
{
	lprc->left   = 0;
	lprc->top    = 0;
	lprc->right  = 2*OFFSET+m_data.nCellSize*m_data.uCountColumn+(m_data.uGridMode ? m_data.uCountColumn+1 : 0)*GRID_WIDTH;
	lprc->bottom = 2*OFFSET+m_data.nCellSize*m_data.uCountRow+(m_data.uGridMode ? m_data.uCountRow+1 : 0)*GRID_WIDTH;
}

BOOL CGame::UpdateBuffer() {
	EnterCriticalSection(&m_data.thread.critsection);

	CalcBufferRect(&m_data.p.rc);
	HBITMAP hBitmap = (HBITMAP)CreateCompatibleBitmap(m_data.p.hDC, m_data.p.rc.right, m_data.p.rc.bottom);
	if (hBitmap != NULL)
	{
		DeleteObject(SelectObject(m_data.p.hDC, hBitmap));

		UpdateBitmap(&m_data);
		InvalidateRect(m_data.p.hWnd, nullptr, FALSE);
	}

	LeaveCriticalSection(&m_data.thread.critsection);
	return (hBitmap != NULL);
}

BOOL CGame::CreateBuffer()
{
	CalcBufferRect(&m_data.p.rc);

	ReleaseBuffer();
	HDC hDC = GetDC(m_data.p.hWnd);
	if (!hDC)
		return FALSE;

	m_data.p.hDC = CreateCompatibleDC(hDC);
	ReleaseDC(m_data.p.hWnd, hDC);
	if (!m_data.p.hDC)
		return FALSE;

	HBITMAP hBitmap = (HBITMAP)CreateCompatibleBitmap(m_data.p.hDC, m_data.p.rc.right, m_data.p.rc.bottom);
	if (hBitmap != NULL)
	{
		DeleteObject(SelectObject(m_data.p.hDC, hBitmap));
		m_data.map = new BOOL[m_data.uCountRow * m_data.uCountColumn]{0};
	}
	else
		ReleaseBuffer();
	return (hBitmap != NULL);
}

void CGame::ReleaseBuffer()
{
	if (m_data.p.hDC)
	{
		HBITMAP hBitmap = (HBITMAP)GetCurrentObject(m_data.p.hDC, OBJ_BITMAP);
		DeleteDC(m_data.p.hDC);
		m_data.p.hDC = NULL;
		DeleteObject(hBitmap);
	}

	if (m_data.map)
	{
		delete[] m_data.map;
		m_data.map = nullptr;
	}
}

BOOL CGame::Init(HWND hWndParent, int sz, UINT cols, UINT rows)
{
	if (m_data.thread.hThread)
		return FALSE;

	m_data.nCellSize	 = sz;
	m_data.uCountColumn  = cols;
	m_data.uCountRow	 = rows;
	m_data.p.hWnd   	 = hWndParent;
	m_data.thread.bPause = TRUE;
	m_data.uGridMode     = GameGridMode::Grid;

	m_ptOffset = { 0, 0 };

	m_data.thread.hThread = CreateThread(nullptr, 0, ThreadProc, &m_data, 0, &m_data.thread.dwThreadID);

	if (!cols || !rows || sz <= 0 || !CreateBuffer()) {
		MessageBox(NULL, L"Не удалось создать поле заданных размеров!", L"Ошибка", MB_OK);
		return FALSE;
	}
	return (m_data.thread.hThread != NULL);
}

void CGame::Random(float p) {
	if (!CheckPause())
		return;

	p = 1-p;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist;
	for (UINT i = 0; i < m_data.uCountColumn*m_data.uCountRow; i++)
		m_data.map[i] = (dist(gen) > p);
	Update(FALSE);
}

void CGame::SetGridMode(UINT uGridMode) {
	m_data.uGridMode = uGridMode;
	Update(FALSE);
}

void CGame::SetOffset(int dx, int dy) { 
	m_ptOffset.x += dx;
	m_ptOffset.y += dy;

	EnterCriticalSection(&m_data.thread.critsection);
	InvalidateRect(m_data.p.hWnd, nullptr, FALSE);
	LeaveCriticalSection(&m_data.thread.critsection);
}

void CGame::SetDelay(int nDelay) { m_data.thread.nDelay = nDelay; }
void CGame::Pause(BOOL bPause) { m_data.thread.bPause = bPause; }
void CGame::GetBufferRect(LPRECT lprc) { *lprc = m_data.p.rc; }
BOOL CGame::IsPause() { return m_data.thread.bPause; }
UINT CGame::GetGridMode() { return m_data.uGridMode; }
int CGame::GetDelay() { return m_data.thread.nDelay; }
int CGame::GetCellSize() { return m_data.nCellSize; }

void CGame::SetZoom(int nCellSize, int x, int y) {
	if (nCellSize <= 0)
		return;
	m_data.nCellSize = nCellSize;

	RECT rcNew, rcWindow;
	CalcBufferRect(&rcNew);
	GetClientRect(m_data.p.hWnd, &rcWindow);
	m_ptOffset.x += (float(rcNew.right-m_data.p.rc.right)*x)/rcWindow.right;
	m_ptOffset.y += (float(rcNew.bottom-m_data.p.rc.bottom)*y)/rcWindow.bottom;

	UpdateBuffer();
}

BOOL CGame::Save() {
	if (!m_data.p.hDC || !m_data.p.hWnd) {
		MessageBoxW(NULL, L"В данный момент сохранение невозможно!", L"Ошибка", MB_OK);
		return FALSE;
	}

	EnterCriticalSection(&m_data.thread.critsection);
	int nResult = SaveBitmapFromHDC(m_data.p.hDC, L"screenshot.bmp");
	LeaveCriticalSection(&m_data.thread.critsection);
	return (nResult == 0);
}

void CGame::Stop()
{
	if (!m_data.thread.hThread)
		return;

	int nDelay = m_data.thread.nDelay+100;
	m_data.thread.nDelay = 0;
	Sleep(nDelay);

	CloseHandle(m_data.thread.hThread);
	m_data.thread.hThread = NULL;
}

// ========= ========= ========= ========= ========= ========= ========= =========

BOOL IsAlive(THREADDATA* pData, int x, int y) {
	if ((x < 0 || x >= (int)pData->uCountColumn) ||
		(y < 0 || y >= (int)pData->uCountRow))
		return 0;
	return pData->map[(y*pData->uCountColumn)+x];
}

inline void Step(THREADDATA* pData)
{
	UINT uPos = 0;
	UINT uCountAlive = 0;
	BOOL* map = new BOOL[pData->uCountRow*pData->uCountColumn]{0};
	for (int y = 0; y < (int)pData->uCountRow; y++)
	{
		for (int x = 0; x < (int)pData->uCountColumn; x++)
		{
			if (pData->thread.nDelay < 1 || pData->thread.bPause)
			{
				delete[] map;
				return;
			}

			uCountAlive = IsAlive(pData, x-1, y-1) + IsAlive(pData, x, y-1) + IsAlive(pData, x+1, y-1) +
						  IsAlive(pData, x-1,  y)							+ IsAlive(pData, x+1,  y)  +
						  IsAlive(pData, x-1, y+1) + IsAlive(pData, x, y+1) + IsAlive(pData, x+1, y+1);

			if (pData->map[uPos] && (uCountAlive < 2 || uCountAlive > 3))
				map[uPos] = FALSE;
			else if (!pData->map[uPos] && uCountAlive == 3)
				map[uPos] = TRUE;
			else
 				map[uPos] = pData->map[uPos];
			uPos++;
		}
	}
	delete[] pData->map;
	pData->map = map;
}

inline void UpdateBitmap(THREADDATA* pData)
{
	if (pData->p.hDC == NULL || pData->nCellSize <= 0)
		return;

	UINT uPos = 0;
	UINT uOffset = pData->nCellSize + (pData->uGridMode ? GRID_WIDTH : 0);
	RECT rcCell = { OFFSET, OFFSET, OFFSET+(LONG)pData->nCellSize, OFFSET+(LONG)pData->nCellSize };
	FillRect(pData->p.hDC, &pData->p.rc, pData->brush.bkg);
	
	if (pData->uGridMode == GameGridMode::Grid) {
		for (UINT x = 0; x <= pData->uCountColumn; x++) {
			MoveToEx(pData->p.hDC, OFFSET+x*(pData->nCellSize+GRID_WIDTH)-1, OFFSET-1, nullptr);
			LineTo(pData->p.hDC, OFFSET+x*(pData->nCellSize+GRID_WIDTH)-1, pData->p.rc.bottom-OFFSET-1);
		}
		for (UINT y = 0; y <= pData->uCountRow; y++) {
			MoveToEx(pData->p.hDC, OFFSET-1, OFFSET+y*(pData->nCellSize+GRID_WIDTH)-1, nullptr);
			LineTo(pData->p.hDC, pData->p.rc.right-OFFSET-1, OFFSET+y*(pData->nCellSize+GRID_WIDTH)-1);
		}
	}

	for (UINT y = 0; y < pData->uCountRow; y++)
	{
		for (UINT x = 0; x < pData->uCountColumn; x++)
		{
			if (pData->thread.nDelay < 1)
				return;

			if (pData->map[uPos])
			{
				if (pData->nCellSize == 1)
					SetPixel(pData->p.hDC, rcCell.left, rcCell.top, 0 /*Black*/);
				else
					FillRect(pData->p.hDC, &rcCell, pData->brush.cell);
			}
			rcCell.right += uOffset;
			rcCell.left  += uOffset;
			uPos++;
		}
		rcCell.bottom += uOffset;
		rcCell.top	  += uOffset;
		rcCell.left    = OFFSET;
		rcCell.right   = OFFSET+(LONG)pData->nCellSize;
	}
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	THREADDATA* pData = (THREADDATA*)lpParam;
	pData->thread.nDelay = 200;
	while (pData->thread.nDelay > 0)
	{
		if (pData->thread.bPause) {
			Sleep(100);
			continue;
		}

		EnterCriticalSection(&pData->thread.critsection);

		Step(pData);
		UpdateBitmap(pData);
		if (!pData->thread.bPause)
			InvalidateRect(pData->p.hWnd, nullptr, FALSE);

		LeaveCriticalSection(&pData->thread.critsection);

		Sleep(pData->thread.nDelay);
	}
	MessageBox(NULL, L"Shutdown...", L"Exit", MB_OK);
	return 0;
}