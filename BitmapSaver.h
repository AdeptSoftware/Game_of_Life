#pragma once // https://docs.microsoft.com/ru-ru/windows/win32/gdi/storing-an-image?redirectedfrom=MSDN
             // https://www.cyberforum.ru/win-api/thread1521896.html
#include <Windows.h>

class CGlobalPtr {
public:
    CGlobalPtr(UINT uFlags, SIZE_T dwBytes) { m_ptr = (LPBYTE)GlobalAlloc(uFlags, dwBytes); }
    ~CGlobalPtr() { 
        GlobalFree((HGLOBAL)m_ptr); }
    LPVOID get() { return m_ptr; }

private:
    LPVOID m_ptr;
};

// Вернет 0 при успешном завершении
int SaveBitmapFromHDC(HDC hDC, LPCTSTR pszFile) {
    DWORD dwBPP = GetDeviceCaps(hDC, BITSPIXEL);
    if (dwBPP <= 8)
        return 1;

    BITMAP bmp;
    HBITMAP hBitmap = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);
    if (!GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bmp))
        return 2;

    BITMAPINFO bmInfo;
    bmInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmInfo.bmiHeader.biWidth         = bmp.bmWidth;
    bmInfo.bmiHeader.biHeight        = bmp.bmHeight;
    bmInfo.bmiHeader.biPlanes        = bmp.bmPlanes;
    bmInfo.bmiHeader.biBitCount      = (WORD)dwBPP;
    bmInfo.bmiHeader.biSizeImage     = ((bmp.bmWidth * dwBPP + 31) & ~31) / 8 * bmp.bmHeight;
    bmInfo.bmiHeader.biCompression   = BI_RGB;
    bmInfo.bmiHeader.biXPelsPerMeter = 0;
    bmInfo.bmiHeader.biYPelsPerMeter = 0;
    bmInfo.bmiHeader.biClrImportant  = 0;
    bmInfo.bmiHeader.biClrUsed       = 0;

    CGlobalPtr pBits(GMEM_FIXED, bmInfo.bmiHeader.biSizeImage);
    if (!pBits.get())
        return 3;

    // Retrieve the color table (RGBQUAD array) and the bits (array of palette indices) from the DIB.  
    if (!GetDIBits(hDC, hBitmap, 0, (WORD)bmp.bmHeight, pBits.get(), &bmInfo, DIB_RGB_COLORS))
        return 4;

    DWORD dwSize = (bmp.bmWidth * bmp.bmHeight * dwBPP) / 8;

    BITMAPFILEHEADER bmfh;
    bmfh.bfType      = 0x4D42; // 0x42 = "B" 0x4d = "M"   // Compute the size of the entire file.  
    bmfh.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfh.bfSize      = dwSize + bmfh.bfOffBits;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
 
    HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 5;

    if (!WriteFile(hFile, (LPVOID)&bmfh, sizeof(BITMAPFILEHEADER), NULL, NULL) ||
        !WriteFile(hFile, (LPVOID)&bmInfo.bmiHeader, sizeof(BITMAPINFOHEADER), NULL, NULL) ||
        !WriteFile(hFile, pBits.get(), dwSize, NULL, NULL))
        return 6;

    return (CloseHandle(hFile) ? 0 : 7);
}