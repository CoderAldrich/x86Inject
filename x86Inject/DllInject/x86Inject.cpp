#include "x86Inject.h"


HANDLE WINAPI ThreadProc(PTHREAD_DATA data)
{
	data->fnRtlInitUnicodeString(&data->UnicodeString,data->DllName);
	data->fnLdrLoadDll(data->DllPath,data->Flags,&data->UnicodeString,&data->ModuleHandle);
	return data->ModuleHandle;
}

DWORD WINAPI ThreadProcEnd()
{
	MyOutputDebugStringA("ThreadProcEnd");
	return 0;
}


Cx86Inject::Cx86Inject(void)
{
}


Cx86Inject::~Cx86Inject(void)
{
}

//����ϵͳ�汾�ж�
BOOL Cx86Inject::IsVistaOrLater()
{
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if( osvi.dwMajorVersion >= 6 )
		return TRUE;
	return FALSE;
}

BOOL Cx86Inject::EnableDebugPrivilege()
{
	HANDLE hToken;   
	TOKEN_PRIVILEGES tkp;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))   
		return( FALSE );
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;   
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	if (GetLastError() != ERROR_SUCCESS)   
		return FALSE;   

	return TRUE; 
}


//OD���٣����������õ���NtCreateThreadEx,���������ֶ�����
HANDLE Cx86Inject::MyCreateRemoteThread(HANDLE hProcess, LPTHREAD_START_ROUTINE pThreadProc, LPVOID pRemoteBuf)
{
	HANDLE hThread = NULL;
	FARPROC pFunc = NULL;
	if( IsVistaOrLater())// Vista, 7, Server2008
	{
		pFunc = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtCreateThreadEx");
		if( pFunc == NULL )
		{
			MyOutputDebugStringA("MyCreateRemoteThread() : GetProcAddress(\"NtCreateThreadEx\") ����ʧ�ܣ��������: [%d]",	GetLastError());
			return NULL;
		}
		((_NtCreateThreadEx32)pFunc)(&hThread,0x1FFFFF,NULL,hProcess,pThreadProc,pRemoteBuf,FALSE,NULL,NULL,NULL,NULL);

		if( hThread == NULL )
		{
			MyOutputDebugStringA("MyCreateRemoteThread() : NtCreateThreadEx() ����ʧ�ܣ��������: [%d]", GetLastError());
			return NULL;
		}
	}
	else// 2000, XP, Server2003
	{
		hThread = CreateRemoteThread(hProcess,NULL,0,pThreadProc,pRemoteBuf,0,NULL);
		if( hThread == NULL )
		{
			MyOutputDebugStringA("MyCreateRemoteThread() : CreateRemoteThread() ����ʧ�ܣ��������: [%d]", GetLastError());
			return NULL;
		}
	}
	if( WAIT_FAILED == WaitForSingleObject(hThread, INFINITE) )
	{
		MyOutputDebugStringA("MyCreateRemoteThread() : WaitForSingleObject() ����ʧ�ܣ��������: [%d]", GetLastError());
		return NULL;
	}
	return hThread;
}

//��Ŀ������д����̲߳�ע��dll
BOOL Cx86Inject::InjectDll(DWORD dwProcessId,LPCWSTR lpcwDllPath)
{
	BOOL bRet = FALSE;
	HANDLE hProcess = NULL, hThread = NULL;
	LPVOID pCode = NULL;
	LPVOID pThreadData = NULL;
	__try
	{
		if(!EnableDebugPrivilege())
		{
			MyOutputDebugStringA("EnableDebugPrivilege Failed!");
			return -1;
		}
		//��Ŀ�����;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwProcessId);
		DWORD dwError = GetLastError();
		if (hProcess == NULL)
			__leave;
		//����ռ䣬�����ǵĴ��������д��Ŀ����̿ռ���;
		//д������;
		THREAD_DATA data;
		HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
		data.fnRtlInitUnicodeString = (pRtlInitUnicodeString)GetProcAddress(hNtdll,"RtlInitUnicodeString");
		data.fnLdrLoadDll = (pLdrLoadDll)GetProcAddress(hNtdll,"LdrLoadDll");
		memcpy(data.DllName, lpcwDllPath, (wcslen(lpcwDllPath) + 1)*sizeof(WCHAR));
		data.DllPath = NULL;
		data.Flags = 0;
		data.ModuleHandle = INVALID_HANDLE_VALUE;
		pThreadData = VirtualAllocEx(hProcess, NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (pThreadData == NULL)
			__leave;
		BOOL bWriteOK = WriteProcessMemory(hProcess, pThreadData,&data,sizeof(data), NULL);
		if (!bWriteOK)
			__leave;
		MyOutputDebugStringA("pThreadData = 0x%p",pThreadData);
		//д�����;
		DWORD SizeOfCode = (DWORD)ThreadProcEnd - (DWORD)ThreadProc;
		pCode = VirtualAllocEx(hProcess, NULL, SizeOfCode, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (pCode == NULL)
			__leave;
		bWriteOK = WriteProcessMemory(hProcess, pCode, (PVOID)ThreadProc, SizeOfCode, NULL);
		if (!bWriteOK)
			__leave;
		MyOutputDebugStringA("pCode = 0x%p",pCode);
		//����Զ���̣߳���ThreadProc��Ϊ�߳���ʼ������pThreadData��Ϊ����;
		hThread = MyCreateRemoteThread(hProcess, (LPTHREAD_START_ROUTINE)pCode, pThreadData);
		if (hThread == NULL)
			__leave;
		//�ȴ����;
		WaitForSingleObject(hThread, INFINITE);
		bRet = TRUE;
	}
	__finally
	{
		if (pThreadData != NULL)
			VirtualFreeEx(hProcess, pThreadData, 0, MEM_RELEASE);
		if (pCode != NULL)
			VirtualFreeEx(hProcess, pCode, 0, MEM_RELEASE);
		if (hThread != NULL)
			CloseHandle(hThread);
		if (hProcess != NULL)
			CloseHandle(hProcess);
	}

	return bRet;
}

