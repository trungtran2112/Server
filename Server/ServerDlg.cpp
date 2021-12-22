
// ServerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool load_data(SLList& L);
UINT update_database(LPVOID param);
UINT handle_client(LPVOID param);

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CServerDlg dialog



CServerDlg::CServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SERVER_LOG, _list_server_log);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_LISTEN, &CServerDlg::OnBnClickedBtnListen)
	ON_BN_CLICKED(IDC_BTN_SHUTDOWN, &CServerDlg::OnBnClickedBtnShutdown)
END_MESSAGE_MAP()


// CServerDlg message handlers

BOOL CServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	//Initialize Winsock
	WSADATA wsaDATA;
	if (WSAStartup(MAKEWORD(2, 2), &wsaDATA) != 0)	//Initial winsock's version
	{
		MessageBox(_T("Không thể khởi tạo Winsock! Hãy thử lại"), _T("Lỗi"), MB_ICONSTOP);
		return FALSE;
	}
	else
	{
		_list_server_log.AddString(_T("Hệ thống: Khởi tạo Winsock thành công!"));
	}

	//Load data from data base
	if (load_data(working_list) == true)
	{
		_list_server_log.AddString(_T("Hệ thống: Lấy dữ liệu thành công!"));
	}
	else
	{
		MessageBox(_T("Không thể lấy dữ liệu! Hãy thử lại"), _T("Lỗi"), MB_ICONSTOP);
	}

	//Update database every 30 minutes
	update_database_thread = AfxBeginThread(update_database, this);




	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CServerDlg::OnBnClickedBtnListen()
{
	// TODO: Add your control notification handler code here
	UpdateData();

	//tao listen socket
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == INVALID_SOCKET)
	{
		MessageBox(_T("Không thể khởi tạo Socket để lắng nghe! Hãy thử lại."), _T("Lỗi"), MB_ICONSTOP);
		return;
	}

	//server info
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);	// host (byte order) to network (byte order 2byte)
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);// host (byte order) to network (byte order 4byte)

	bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr));
	listen(listen_socket, 5);
	_list_server_log.AddString(_T("Đang lắng nghe kết nối..."));

	AfxBeginThread(handle_client, this);

	GetDlgItem(IDC_BTN_LISTEN)->EnableWindow(false);

}

UINT handle_client(LPVOID param)
{
	CServerDlg* ptr = (CServerDlg*)param;
	while (true)
	{
		
	}

	return 0;
}

double string_to_double(std::string s)
{
	double value = 0;
	int index = 0;
	for (index; index < s.size(); index++)
	{
		if (s[index] == ',')
			continue;
		if (s[index] == '.')
			break;
		value = value * 10 + (s[index] - '0');
	}
	if (index == s.size() - 1)
		return value;
	index++;
	int decimal_level = 1;
	for (index; index < s.size(); index++)
	{
		decimal_level *= 10;
		value = value + (s[index] - '0') / (double)decimal_level;
	}
	return value;
}

Node* add_tail(SLList& L)
{
	Node* newNode = new Node;
	if (newNode == NULL)
		return NULL;
	if (L.pHead == NULL)
		L.pHead = L.pTail = newNode;
	else
	{
		L.pTail->pNext = newNode;
		L.pTail = newNode;
	}
	return newNode;
}

size_t callback(char* buffer, size_t size, size_t num, std::string* json_data)
{
	size_t totalBytes = size * num;
	json_data->append(buffer, totalBytes);
	return totalBytes;
}

bool load_data(SLList& L)
{
	std::ifstream json_file_stream("tygia.json");
	Json::Reader json_reader;
	Json::Value json_data;
	if (!json_reader.parse(json_file_stream, json_data))
		return FALSE;
	for (unsigned int i = 0; i < json_data.size(); i++)
	{
		for (unsigned int j = 0; j < json_data[i]["value"].size(); j++)
		{
			Node* newNode = add_tail(L);
			newNode->data.brand = json_data[i]["value"][j]["brand"].asString();
			newNode->data.buy = string_to_double(json_data[i]["value"][j]["buy"].asString());
			newNode->data.company = json_data[i]["value"][j]["company"].asString();
			newNode->data.date = stoi(json_data[i]["value"][j]["date"].asString());
			newNode->data.sell = string_to_double(json_data[i]["value"][j]["sell"].asString());
			newNode->data.type = json_data[i]["value"][j]["type"].asString();
		}
	}
	return TRUE;
}

void delete_list(SLList& L)
{
	if (L.pHead == NULL)
		return;
	Node* current_node = L.pHead, * temp_node = NULL;
	while (current_node != NULL)
	{
		temp_node = current_node;
		current_node = current_node->pNext;
		delete temp_node;
	}
	L.pHead = L.pTail = NULL;
}

UINT update_database(LPVOID Param)
{
	CServerDlg* ptr = (CServerDlg*)Param;
	while (true)
	{


		ptr->_list_server_log.AddString(_T("Dữ liệu của hôm nay vừa được cập nhật!"));
		Sleep(1800000);//sleep for 1 800 000 seconds ~ 30 minutes
	}
	return 0;
}


void CServerDlg::OnBnClickedBtnShutdown()
{
	// TODO: Add your control notification handler code here
	delete_list(working_list);
}
