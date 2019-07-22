
// DllInjectDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "DllInject.h"
#include "DllInjectDlg.h"
#include "afxdialogex.h"

#include "x86Inject.h"
#include <shlwapi.h>
#pragma comment (lib, "shlwapi.lib")

DWORD g_dwPID = 0;
WCHAR g_szDllPath[MAX_PATH] = {0};
CWnd *pBtnInject = NULL;

BOOL AllowMeesageForVistaAbove(UINT uMessage,BOOL bAllow)
{
	typedef BOOL (WINAPI* _ChangeWindowMessageFilter)(UINT, DWORD);
	HMODULE hUserMod = LoadLibraryW(L"user32.dll");
	if (hUserMod == NULL)
	{       
		return FALSE;
	}
	_ChangeWindowMessageFilter pChangeWindowMessageFilter = (_ChangeWindowMessageFilter)GetProcAddress(hUserMod, "ChangeWindowMessageFilter");
	if (NULL == pChangeWindowMessageFilter)
	{       
		FreeLibrary(hUserMod);
		return FALSE;
	}
	BOOL bResult = pChangeWindowMessageFilter(uMessage, bAllow ? 1 : 2); // 1-MSGFLT_ADD, 2-MSGFLT_REMOVE
	if(NULL != hUserMod)
	{
		FreeLibrary(hUserMod);
	}
	return bResult;
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDllInjectDlg �Ի���




CDllInjectDlg::CDllInjectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDllInjectDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDllInjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDllInjectDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_INJECT, &CDllInjectDlg::OnBnClickedBtnInject)
	ON_BN_CLICKED(IDC_BTN_BROWSE, &CDllInjectDlg::OnBnClickedBtnBrowse)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// CDllInjectDlg ��Ϣ�������

BOOL CDllInjectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	//UAC��ק����
	AllowMeesageForVistaAbove(WM_DROPFILES,TRUE);
	AllowMeesageForVistaAbove(0x0049, TRUE ); // 0x0049 - WM_COPYGLOBALDATA

	CString strPath = L"C:\\Windows\\System32\\sfc.dll";
	SetDlgItemText(IDC_EDT_DLLPATH,strPath);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CDllInjectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CDllInjectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CDllInjectDlg::OnBnClickedBtnInject()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString strPID;
	GetDlgItemText(IDC_EDT_PID,strPID);
	strPID.Replace(L" ", L"");
	if (strPID == L"")
	{
		AfxMessageBox(L"����дPID��");
		return;	
	}	
	DWORD dwPID = GetDlgItemInt(IDC_EDT_PID);
	if (dwPID%4 != 0)
	{
		AfxMessageBox(L"PID����ȷ��");
		return;		
	}
	WCHAR szDLLPath[MAX_PATH] = {0};
	GetDlgItemTextW(IDC_EDT_DLLPATH,szDLLPath,MAX_PATH);
	if (!PathFileExistsW(szDLLPath))
	{
		AfxMessageBox(L"DLL�����ڣ�");
		return;
	}
	g_dwPID = dwPID;
	memcpy(g_szDllPath,szDLLPath,MAX_PATH*sizeof(WCHAR));
	AfxBeginThread(ThreadProc,this);
	pBtnInject = GetDlgItem(IDC_BTN_INJECT);
	pBtnInject->EnableWindow(FALSE);
}


void CDllInjectDlg::OnBnClickedBtnBrowse()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CFileDialog dlg(TRUE,NULL,NULL,6UL,_T("��̬���ӿ�(*.DLL)|*.DLL||"));
	if (dlg.DoModal()==IDOK)
	{
		CString strPath = dlg.GetPathName();
		SetDlgItemText(IDC_EDT_DLLPATH,strPath);
	}
}


void CDllInjectDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ	
	UINT uCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);      
	for(UINT i=0; i<uCount; i++)
	{
		char szFilePath[MAX_PATH] = {0};
		DragQueryFileA(hDropInfo, 0, szFilePath, MAX_PATH);
		CStringA strFilePath;
		strFilePath.Format("%s",szFilePath);	
		if (strFilePath.Right(4).MakeUpper() == ".DLL")
		{
			SetDlgItemTextA(m_hWnd,IDC_EDT_DLLPATH,strFilePath);
			break;
		}
	}
	DragFinish(hDropInfo);

	CDialogEx::OnDropFiles(hDropInfo);
}


UINT CDllInjectDlg::ThreadProc(LPVOID lpVoid)
{
	Cx86Inject inject;
	if (inject.InjectDll(g_dwPID,g_szDllPath))
	{
		::MessageBox(NULL,L"ע��ɹ�",L":)",MB_ICONINFORMATION);
	}
	else
	{
		::MessageBox(NULL,L"ע��ʧ�ܣ�",L"ʧ��",MB_ICONERROR);
	}
	pBtnInject->EnableWindow(TRUE);
	return 0;
}


BOOL CDllInjectDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: �ڴ����ר�ô����/����û���
	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		return TRUE; 
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}
