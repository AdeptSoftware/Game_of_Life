#pragma once

#ifdef _DEBUG
#include <debugapi.h>
#include <stdarg.h>
#include <tchar.h>

void OutputDebug(const TCHAR* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int cnt = _vtprintf_s(fmt, args);
	TCHAR* str = new TCHAR[cnt + 1]{ 0 };
	_vsntprintf_s(str, cnt + 1, cnt, fmt, args);
	OutputDebugString(str);
	delete[] str;
	va_end(args);
}

#endif