
// ServerDlg.h : header file
//

#pragma once

#define PORT 2831
#define WM_SOCKET WM_USER+1

struct Element
{
	double buy = 0, sell = 0;
	int date = 0;
	std::string company, brand, type;
};

struct Node
{
	Element data;
	Node* pNext = NULL;
};

struct SLList
{
	Node* pHead = NULL, * pTail = NULL;
};


// CServerDlg dialog
class CServerDlg : public CDialogEx
{
	// Construction
public:
	CServerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

//new function
public:
	afx_msg void OnBnClickedBtnListen();
	afx_msg void OnBnClickedBtnShutdown();
	//UINT handle_client(LPVOID param);
	CListBox _list_server_log;

//newvariables
public:
	CWinThread* update_database_thread;
	SLList working_list;
	SOCKET listen_socket;
	sockaddr_in server_addr;


	int id = 0, client_addr_len[100];
	SOCKET client_socket[100];
	sockaddr_in client_addr[100];
	CWinThread* client_thread[100];


	afx_msg void OnClose();
};
