// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

HINSTANCE g_Inst = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	// 消息流为 1 2 ... 2 3

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
		g_Inst = hModule;
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		//g_Inst = NULL;
		break;
	}
	return TRUE;
}

