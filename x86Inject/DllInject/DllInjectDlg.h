
// DllInjectDlg.h : ͷ�ļ�
//

#pragma once


// CDllInjectDlg �Ի���
class CDllInjectDlg : public CDialogEx
{
// ����
public:
	CDllInjectDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DLLINJECT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnInject();
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	static UINT ThreadProc(LPVOID lpVoid);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
