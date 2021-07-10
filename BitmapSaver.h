#pragma once // https://docs.microsoft.com/ru-ru/windows/win32/gdi/storing-an-image?redirectedfrom=MSDN
             // https://www.cyberforum.ru/win-api/thread1521896.html
#include <Windows.h>

class CGlobalPtr {
public:
    CGlobalPtr(UINT uFlags, SIZE_T dwBytes) { m_ptr = GlobalAlloc(uFlags, dwBytes); }
    ~CGlobalPtr() { 
        GlobalFree(m_ptr); }
    HGLOBAL get() { return m_ptr; }

private:
    HGLOBAL m_ptr;
};

// Вернет 0 при успешном завершении
int SaveBitmapFromHDC(HDC hDC, LPCTSTR pszFile) {
    int nBPP = GetDeviceCaps(hDC, BITSPIXEL);
    if (nBPP <= 8)
        return 1;

    BITMAP bmp;
    HBITMAP hBitmap = reinterpret_cast<HBITMAP>(GetCurrentObject(hDC, OBJ_BITMAP));
    if (!GetObject(hBitmap, sizeof(BITMAP), reinterpret_cast<LPVOID>(&bmp)))
        return 2;

    BITMAPINFO bmInfo;
    bmInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmInfo.bmiHeader.biWidth         = bmp.bmWidth;
    bmInfo.bmiHeader.biHeight        = bmp.bmHeight;
    bmInfo.bmiHeader.biPlanes        = bmp.bmPlanes;
    bmInfo.bmiHeader.biBitCount      = static_cast<WORD>(nBPP);
    bmInfo.bmiHeader.biSizeImage     = ((nBPP * bmp.bmWidth + 31) & ~31) / 8 * bmp.bmHeight;
    bmInfo.bmiHeader.biCompression   = BI_RGB;
    bmInfo.bmiHeader.biXPelsPerMeter = 0;
    bmInfo.bmiHeader.biYPelsPerMeter = 0;
    bmInfo.bmiHeader.biClrImportant  = 0;
    bmInfo.bmiHeader.biClrUsed       = 0;

    CGlobalPtr pBits(GMEM_FIXED, bmInfo.bmiHeader.biSizeImage);
    if (!pBits.get())
        return 3;

    // Retrieve the color table (RGBQUAD array) and the bits (array of palette indices) from the DIB.  
    if (!GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, reinterpret_cast<LPVOID>(pBits.get()), &bmInfo, DIB_RGB_COLORS))
        return 4;

    DWORD dwSize = (nBPP * bmp.bmWidth * bmp.bmHeight) / 8;

    BITMAPFILEHEADER bmfh;
    bmfh.bfType      = 0x4D42; // 0x42 = "B" 0x4d = "M"   // Compute the size of the entire file.  
    bmfh.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfh.bfSize      = dwSize + bmfh.bfOffBits;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;

    HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return 5;

    if (!WriteFile(hFile, reinterpret_cast<LPVOID>(&bmfh),             sizeof(BITMAPFILEHEADER), nullptr, nullptr) ||
        !WriteFile(hFile, reinterpret_cast<LPVOID>(&bmInfo.bmiHeader), sizeof(BITMAPINFOHEADER), nullptr, nullptr) ||
        !WriteFile(hFile, reinterpret_cast<LPVOID>(pBits.get()),       dwSize,                   nullptr, nullptr))
        return 6;

    return (CloseHandle(hFile) ? 0 : 7);
}