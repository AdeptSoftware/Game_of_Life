// Отслеживает утечки памяти, выводя их в консоль.
// Достаточно подключить этот заголовочный файл и всё
#pragma once

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <cstdlib>

	#pragma warning(push)
	#pragma warning(disable:4005)
	#include <crtdbg.h>
	#pragma warning(pop)

	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )

	struct __MEMORYLEAKSDETECTOR
	{
		~__MEMORYLEAKSDETECTOR() { _CrtDumpMemoryLeaks(); }
	}__g_memory_leaks_detector;

#else
	#define DBG_NEW new
#endif
