// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		MessageBoxA(NULL,"Dll Attach!",":)",MB_ICONINFORMATION);
		break;
	case DLL_PROCESS_DETACH:
		MessageBoxA(NULL,"Dll Detach!",":)",MB_ICONINFORMATION);
		break;
	default:
		break;
	}
	return TRUE;
}

