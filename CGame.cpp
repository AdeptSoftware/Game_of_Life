#include "CGame.h"
#include "BitmapSaver.h"
#include <random>

// Константы отображения сетки
const int g_nOffset	   = 2;	// Отступ сетки от края
const int g_nGridWidth = 1;

DWORD WINAPI ThreadProc(LPVOID lpParam);
inline void UpdateBitmap(THREADDATA* pData);

void CGame::Invalidate(BOOL bErase) {
	EnterCriticalSection(&m_data.thread.critsection);

	UpdateBitmap(&m_data);
	InvalidateRect(m_data.p.hWnd, nullptr, bErase);

	LeaveCriticalSection(&m_data.thread.critsection);
}

CGame::CGame() {
	m_data.thread.nDelay	 = 0;
	m_data.thread.dwThreadID = 0;
	m_data.thread.hThread	 = nullptr;
	m_data.thread.bPause	 = FALSE;

	m_data.uGridMode	     = GameGridMode::None;
	m_data.nCellSize		 = 0;
	m_data.nCountRow		 = 0;
	m_data.nCountColumn		 = 0;

	m_ptOffset		    = { 0.0f, 0.0f };
	
	m_data.p.hWnd	    = nullptr;
	m_data.p.hDC	    = nullptr;
	m_data.p.rc		    = { 0, 0, 0, 0 };
	m_data.brush.cell   = CreateSolidBrush(RGB(0, 0, 0));
	m_data.brush.bkg    = CreateSolidBrush(RGB(255, 255, 255));

	InitializeCriticalSection(&m_data.thread.critsection);
}

CGame::~CGame() {
	Stop();
	ReleaseBuffer();
	DeleteObject(m_data.brush.bkg);
	DeleteObject(m_data.brush.cell);
	DeleteCriticalSection(&m_data.thread.critsection);
}

HDC CreateBuffer(HDC hWindowDC, HFONT hFont, int cx, int cy) {
	HDC hBufferDC = CreateCompatibleDC(hWindowDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC, cx, cy);
	DeleteObject(SelectObject(hBufferDC, hBitmap));
	DeleteObject(SelectObject(hBufferDC, hFont));
	return hBufferDC;
}

void DeleteBuffer(HDC hWindowDC, HDC hBufferDC, int x, int y, int cx, int cy) {
	BitBlt(hWindowDC, 0, 0, cx, cy, hBufferDC, x, y, SRCCOPY);
	HBITMAP hBitmap = reinterpret_cast<HBITMAP>(GetCurrentObject(hBufferDC, OBJ_BITMAP));
	DeleteDC(hBufferDC);
	DeleteObject(hBitmap);
}

void CGame::Draw(HWND hWnd, HDC hWindowDC) {
	if (m_data.p.hDC) {
		RECT rcWindow;
		GetClientRect(hWnd, &rcWindow);
		HDC hDC = ::CreateBuffer(hWindowDC, nullptr, rcWindow.right, rcWindow.bottom);

		FillRect(hDC, &rcWindow, m_data.brush.bkg);
		BitBlt(hDC, 1, 1, rcWindow.right, rcWindow.bottom, m_data.p.hDC,
			   static_cast<int>(m_ptOffset.x), static_cast<int>(m_ptOffset.y), SRCCOPY);

		::DeleteBuffer(hWindowDC, hDC, 0, 0, rcWindow.right, rcWindow.bottom);
	}
}

BOOL CGame::CheckPause() {
	if (!m_data.thread.bPause) {
		MessageBox(nullptr, L"Установка доступна только на паузе!", L"Предупреждение!", MB_OK);
		return FALSE;
	}
	return TRUE;
}

void CGame::InvertState(int nPos) {
	if (CheckPause()) {
		if (nPos >= 0 && nPos < m_data.nCountColumn*m_data.nCountRow) {
			m_data.map[nPos] = !m_data.map[nPos];
			Invalidate(FALSE);
		}
	}
}

UINT CGame::Cursor2Pos(LPPOINT lppt) {
	if (!m_data.nCellSize)
		return 0;
	int nGrid = (m_data.uGridMode ? g_nGridWidth : 0);
	int x = (lppt->x+static_cast<int>(m_ptOffset.x)-g_nOffset+nGrid)/(nGrid+m_data.nCellSize);
	int y = (lppt->y+static_cast<int>(m_ptOffset.y)-g_nOffset+nGrid)/(nGrid+m_data.nCellSize);
	if ((x < 0 || x >= m_data.nCountColumn) || (y < 0 || y >= m_data.nCountRow))
		return m_data.nCountColumn*m_data.nCountRow;
	return UINT(y*m_data.nCountColumn+x);
}

void CGame::CalcBufferRect(LPRECT lprc) {
	lprc->left   = 0;
	lprc->top    = 0;
	lprc->right  = 2*g_nOffset+m_data.nCellSize*m_data.nCountColumn+(m_data.uGridMode ? m_data.nCountColumn+1 : 0)*g_nGridWidth;
	lprc->bottom = 2*g_nOffset+m_data.nCellSize*m_data.nCountRow+(m_data.uGridMode ? m_data.nCountRow+1 : 0)*g_nGridWidth;
}

BOOL CGame::UpdateBuffer() {
	EnterCriticalSection(&m_data.thread.critsection);

	CalcBufferRect(&m_data.p.rc);
	HBITMAP hBitmap = CreateCompatibleBitmap(m_data.p.hDC, m_data.p.rc.right, m_data.p.rc.bottom);
	if (hBitmap != nullptr) {
		DeleteObject(SelectObject(m_data.p.hDC, hBitmap));

		UpdateBitmap(&m_data);
		InvalidateRect(m_data.p.hWnd, nullptr, FALSE);
	}

	LeaveCriticalSection(&m_data.thread.critsection);
	return (hBitmap != nullptr);
}

BOOL CGame::CreateBuffer() {
	CalcBufferRect(&m_data.p.rc);

	ReleaseBuffer();
	HDC hDC = GetDC(m_data.p.hWnd);
	if (!hDC)
		return FALSE;

	m_data.p.hDC = CreateCompatibleDC(hDC);
	ReleaseDC(m_data.p.hWnd, hDC);
	if (!m_data.p.hDC)
		return FALSE;

	HBITMAP hBitmap = CreateCompatibleBitmap(m_data.p.hDC, m_data.p.rc.right, m_data.p.rc.bottom);
	if (hBitmap != nullptr) {
		DeleteObject(SelectObject(m_data.p.hDC, hBitmap));
		m_data.map = new BOOL[m_data.nCountRow*m_data.nCountColumn]{0};
		return TRUE;
	}
	else
		ReleaseBuffer();
	return FALSE;
}

void CGame::ReleaseBuffer() {
	if (m_data.p.hDC) {
		HBITMAP hBitmap = reinterpret_cast<HBITMAP>(GetCurrentObject(m_data.p.hDC, OBJ_BITMAP));
		DeleteDC(m_data.p.hDC);
		m_data.p.hDC = nullptr;
		DeleteObject(hBitmap);
	}

	if (m_data.map) {
		delete[] m_data.map;
		m_data.map = nullptr;
	}
}

BOOL CGame::Init(HWND hWndParent, int sz, int cols, int rows) {
	if (m_data.thread.hThread)
		return FALSE;

	m_data.nCellSize	 = sz;
	m_data.nCountColumn  = cols;
	m_data.nCountRow	 = rows;
	m_data.thread.bPause = TRUE;
	m_data.p.hWnd   	 = hWndParent;
	m_data.uGridMode     = GameGridMode::Grid;

	m_ptOffset = { 0, 0 };

	m_data.thread.hThread = CreateThread(nullptr, 0, ThreadProc, &m_data, 0, &m_data.thread.dwThreadID);
	if (!m_data.thread.hThread) {
		MessageBox(nullptr, L"Ошибка инициализации!", L"Ошибка", MB_OK);
		return FALSE;
	}

	if (cols <= 0 || rows <= 0 || sz <= 0 || !CreateBuffer()) {
		MessageBox(nullptr, L"Не удалось создать поле заданных размеров!", L"Ошибка", MB_OK);
		return FALSE;
	}
	return TRUE;
}

void CGame::Random(float p) {
	if (CheckPause()) {
		p = 1-p;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist;
		for (int i = 0; i < m_data.nCountColumn*m_data.nCountRow; i++)
			m_data.map[i] = (dist(gen) > p);
		Invalidate(FALSE);
	}
}

void CGame::SetGridMode(UINT uGridMode) {
	m_data.uGridMode = uGridMode;
	Invalidate(FALSE);
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
UINT CGame::GetGridMode() { return m_data.uGridMode; }
BOOL CGame::IsPause() { return m_data.thread.bPause; }
int CGame::GetDelay() { return m_data.thread.nDelay; }
int CGame::GetCellSize() { return m_data.nCellSize; }

BOOL CGame::IsDefaultOffset() {
	return (m_ptOffset.x == 0.0f && m_ptOffset.y == 0.0f);
}

void CGame::SetZoom(int nCellSize, int x, int y) {
	if (nCellSize > 0) {
		m_data.nCellSize = nCellSize;

		RECT rcNew, rcWindow;
		CalcBufferRect(&rcNew);
		GetClientRect(m_data.p.hWnd, &rcWindow);
		m_ptOffset.x += (float(rcNew.right-m_data.p.rc.right)*x)/rcWindow.right;
		m_ptOffset.y += (float(rcNew.bottom-m_data.p.rc.bottom)*y)/rcWindow.bottom;

		UpdateBuffer();
	}
}

BOOL CGame::Save() {
	if (!m_data.p.hDC || !m_data.p.hWnd) {
		MessageBoxW(nullptr, L"В данный момент сохранение невозможно!", L"Ошибка", MB_OK);
		return FALSE;
	}

	EnterCriticalSection(&m_data.thread.critsection);
	int nResult = SaveBitmapFromHDC(m_data.p.hDC, L"screenshot.bmp");
	LeaveCriticalSection(&m_data.thread.critsection);
	return (nResult == 0);
}

void CGame::Stop() {
	if (!m_data.thread.hThread)
		return;

	int nDelay = m_data.thread.nDelay;
	m_data.thread.nDelay = 0;
	Sleep(nDelay+100); // Немного подождем завершения потока

	CloseHandle(m_data.thread.hThread);
	m_data.thread.hThread = nullptr;
}

// ========= ========= ========= ========= ========= ========= ========= =========

BOOL IsAlive(THREADDATA* pData, int x, int y) {
	if ((x < 0 || x >= pData->nCountColumn) ||
		(y < 0 || y >= pData->nCountRow))
		return 0;
	return pData->map[(y*pData->nCountColumn)+x];
}

inline void Step(THREADDATA* pData) {
	UINT uPos = 0;
	UINT uCountAlive = 0;
	BOOL* map = new BOOL[pData->nCountRow*pData->nCountColumn]{0};
	for (int y = 0; y < pData->nCountRow; y++) {
		for (int x = 0; x < pData->nCountColumn; x++) {
			if (pData->thread.nDelay < 1 || pData->thread.bPause) {
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

void UpdateBitmap(THREADDATA* pData) {
	if (!pData->p.hDC || pData->nCellSize <= 0)
		return;

	UINT uPos = 0;
	int nOffset = pData->nCellSize + (pData->uGridMode ? g_nGridWidth : 0);
	RECT rcCell = { g_nOffset, g_nOffset, g_nOffset+pData->nCellSize, g_nOffset+pData->nCellSize };
	FillRect(pData->p.hDC, &pData->p.rc, pData->brush.bkg);
	
	if (pData->uGridMode == GameGridMode::Grid) {// Отрисовка сетки
		for (int x = 0; x <= pData->nCountColumn; x++) {
			MoveToEx(pData->p.hDC, g_nOffset+x*(pData->nCellSize+g_nGridWidth)-1, g_nOffset-1, nullptr);
			LineTo(pData->p.hDC, g_nOffset+x*(pData->nCellSize+g_nGridWidth)-1, pData->p.rc.bottom-g_nOffset-1);
		}
		for (int y = 0; y <= pData->nCountRow; y++) {
			MoveToEx(pData->p.hDC, g_nOffset-1, g_nOffset+y*(pData->nCellSize+g_nGridWidth)-1, nullptr);
			LineTo(pData->p.hDC, pData->p.rc.right-g_nOffset-1, g_nOffset+y*(pData->nCellSize+g_nGridWidth)-1);
		}
	}

	for (int y = 0; y < pData->nCountRow; y++) {// Отрисовка ячеек
		for (int x = 0; x < pData->nCountColumn; x++) {
			if (pData->thread.nDelay < 1)
				return;

			if (pData->map[uPos])
				FillRect(pData->p.hDC, &rcCell, pData->brush.cell);
			rcCell.right += nOffset;
			rcCell.left  += nOffset;
			uPos++;
		}
		rcCell.bottom += nOffset;
		rcCell.top	  += nOffset;	// Смещение к следующей ячейке
		rcCell.left    = g_nOffset; // Смещение от границ окна
		rcCell.right   = g_nOffset+(LONG)pData->nCellSize;
	}
}

DWORD WINAPI ThreadProc(LPVOID lpParam) {
	THREADDATA* pData = (THREADDATA*)lpParam;
	pData->thread.nDelay = 200;
	while (pData->thread.nDelay > 0) {
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
	MessageBox(nullptr, L"Shutdown...", L"Exit", MB_OK);
	return 0;
}