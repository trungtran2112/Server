
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
UINT client_thread(LPVOID param);
int mSend(CString msg, SOCKET& sock);
int mRecv(CString& StrRecv, SOCKET sClient);
user_node* add_tail(UserList& L);
bool load_username_from_file(UserList& L);
void save_username_to_file(UserList L);
int CharToWchar_t(const char* src, CString& des);

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
	ON_WM_CLOSE()
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

	//Load username
	load_username_from_file(user_list);


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
	_list_server_log.AddString(_T("Hệ thống: Đang lắng nghe kết nối..."));

	handle_client_thread = AfxBeginThread(handle_client, this);

	GetDlgItem(IDC_BTN_LISTEN)->EnableWindow(false);
}

UINT handle_client(LPVOID param)
{
	CServerDlg* ptr = (CServerDlg*)param;
	while (true)
	{

		ptr->client_addr_len[ptr->id + 1] = sizeof(ptr->client_addr[ptr->id + 1]);
		ptr->client_socket[ptr->id + 1] = accept(ptr->listen_socket,
			(sockaddr*)&ptr->client_addr[ptr->id + 1], &ptr->client_addr_len[ptr->id + 1]);
		ptr->id++;
		ptr->client_thread[ptr->id] = AfxBeginThread(client_thread, ptr);
	}

	return 0;
}

UINT client_thread(LPVOID param)
{

	CServerDlg* ptr = (CServerDlg*)param;
	int id = ptr->id;
	CString flag;
	int bytes_received;
	CString client_name;

	ptr->_list_server_log.AddString(_T("Hệ thống: Một client mới đã kết nối."));

	while (true)
	{
		bytes_received = mRecv(flag, ptr->client_socket[id]);

		//client disconnect
		if (bytes_received == 0)
		{
			//client disconnect
			if (client_name != "")
				ptr->_list_server_log.AddString(_T("Hệ thống: ") + client_name + _T(" đã ngắt kết nối."));
			else
				ptr->_list_server_log.AddString(_T("Hệ thống: Một client chưa đăng nhập đã ngắt kết nối."));
			break;
		}

		//error connection 
		else if (bytes_received < 0)
		{
			ptr->_list_server_log.AddString(_T("Hệ thống: Có sự cố xảy ra với ") + client_name + _T("."));
			break;
		}

		int int_flag = _tstoi(flag);

		switch (int_flag)
		{
		case 0:	//log in
		{
			CString username, password;
			mRecv(username, ptr->client_socket[id]);
			mRecv(password, ptr->client_socket[id]);
			user_node* current_node = ptr->user_list.pHead;
			bool check = false;

			while (current_node != NULL)
			{
				if (current_node->username == username && current_node->password == password)
				{

					check = true;
					break;
				}
				current_node = current_node->pNext;
			}

			//0 khong tim duoc tai khoan
			//1 dang nhap thanh cong
			if (check == false)//khong kiem thay ten dang nhap
			{
				mSend(_T("0"), ptr->client_socket[id]);
			}
			else//kiem duoc ten dang nhap va mat khau
			{
				mSend(_T("1"), ptr->client_socket[id]);
				client_name = username;
				ptr->_list_server_log.AddString(_T("Hệ thống: \"") + client_name + _T("\" đã đăng nhập."));
			}
			break;
		}
		case 1: //register
		{
			CString username, password;
			mRecv(username, ptr->client_socket[id]);
			mRecv(password, ptr->client_socket[id]);

			user_node* current_node = ptr->user_list.pHead;
			bool check = true;

			while (current_node != NULL)
			{
				if (current_node->username == username)
				{

					check = false;	//trung` ten dang nhap
					break;
				}
				current_node = current_node->pNext;
			}

			//0 trung` ten dang nhap
			//1 dang ky thanh cong
			if (check == false)
			{
				mSend(_T("0"), ptr->client_socket[id]);
			}
			else
			{
				mSend(_T("1"), ptr->client_socket[id]);
				user_node* newNode = add_tail(ptr->user_list);
				newNode->username = username;
				newNode->password = password;
				save_username_to_file(ptr->user_list);
			}
			break;
		}
		case 2://data query
		{
			bool no_item_found = true;
			CString company, type, brand, date;
			mRecv(company, ptr->client_socket[id]);
			mRecv(type, ptr->client_socket[id]);
			mRecv(brand, ptr->client_socket[id]);
			mRecv(date, ptr->client_socket[id]);

			int a[3] = { 0,0,0 };
			if (company.Compare(_T("Tất cả")) == 0)
				a[0] = 1;
			if (type.Compare(_T("Tất cả")) == 0)
				a[1] = 1;
			if (brand.Compare(_T("Tất cả")) == 0)
				a[2] = 1;
			int search_case = 4 * a[0] + 2 * a[1] + a[2];

			Node* current_node = ptr->working_list.pHead;

			//tim node dau tien cua ngay can tim
			while (current_node != NULL)
			{
				if (current_node->data.date == _ttoi(date))
				{
					break;
				}
				current_node = current_node->pNext;
			}

			//khong tim duoc du lieu nao trong ngay
			if (current_node == NULL)
			{
				mSend(_T("0"), ptr->client_socket[id]);
				break;
			}
			else
			{
				do
				{
					CString nodeCompany, nodeType, nodeBrand, nodeSend;
					CharToWchar_t(current_node->data.company.c_str(), nodeCompany);
					CharToWchar_t(current_node->data.type.c_str(), nodeType);
					CharToWchar_t(current_node->data.brand.c_str(), nodeBrand);
					switch (search_case)
					{
					case 1:
					{
						if (company.Compare(nodeCompany) == 0
							&& type.Compare(nodeType) == 0)
						{
							no_item_found = false;
							mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

							mSend(nodeCompany, ptr->client_socket[id]);
							mSend(nodeType, ptr->client_socket[id]);
							mSend(nodeBrand, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.buy);
							mSend(nodeSend, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.sell);
							mSend(nodeSend, ptr->client_socket[id]);
						}
						break;
					}
					case 2:
					{
						if (company.Compare(nodeCompany) == 0 && brand.Compare(nodeBrand) == 0)
						{
							no_item_found = false;
							mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

							mSend(nodeCompany, ptr->client_socket[id]);
							mSend(nodeType, ptr->client_socket[id]);
							mSend(nodeBrand, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.buy);
							mSend(nodeSend, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.sell);
							mSend(nodeSend, ptr->client_socket[id]);
						}
						break;
					}
					case 3:
					{
						if (company.Compare(nodeCompany) == 0)
						{
							no_item_found = false;
							mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

							mSend(nodeCompany, ptr->client_socket[id]);
							mSend(nodeType, ptr->client_socket[id]);
							mSend(nodeBrand, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.buy);
							mSend(nodeSend, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.sell);
							mSend(nodeSend, ptr->client_socket[id]);
						}
						break;
					}
					case 4:
					{
						if (type.Compare(nodeType) == 0
							&& brand.Compare(nodeBrand) == 0)
						{
							no_item_found = false;
							mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

							mSend(nodeCompany, ptr->client_socket[id]);
							mSend(nodeType, ptr->client_socket[id]);
							mSend(nodeBrand, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.buy);
							mSend(nodeSend, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.sell);
							mSend(nodeSend, ptr->client_socket[id]);
						}
						break;
					}
					case 5:
					{
						if (type.Compare(nodeType) == 0)
						{
							no_item_found = false;
							mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

							mSend(nodeCompany, ptr->client_socket[id]);
							mSend(nodeType, ptr->client_socket[id]);
							mSend(nodeBrand, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.buy);
							mSend(nodeSend, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.sell);
							mSend(nodeSend, ptr->client_socket[id]);
						}
						break;
					}
					case 6:
					{
						if (brand.Compare(nodeBrand) == 0)
						{
							no_item_found = false;
							mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

							mSend(nodeCompany, ptr->client_socket[id]);
							mSend(nodeType, ptr->client_socket[id]);
							mSend(nodeBrand, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.buy);
							mSend(nodeSend, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.sell);
							mSend(nodeSend, ptr->client_socket[id]);
						}
						break;
					}
					case 7:
					{
						no_item_found = false;
						mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

						mSend(nodeCompany, ptr->client_socket[id]);
						mSend(nodeType, ptr->client_socket[id]);
						mSend(nodeBrand, ptr->client_socket[id]);

						nodeSend.Format(_T("%g"), current_node->data.buy);
						mSend(nodeSend, ptr->client_socket[id]);

						nodeSend.Format(_T("%g"), current_node->data.sell);
						mSend(nodeSend, ptr->client_socket[id]);

						break;
					}
					default:
					{
						if (company.Compare(nodeCompany) == 0
							&& type.Compare(nodeType) == 0
							&& brand.Compare(nodeBrand) == 0)
						{
							no_item_found = false;
							mSend(_T("1"), ptr->client_socket[id]);		//gui tin hieu de nhan du lieu

							mSend(nodeCompany, ptr->client_socket[id]);
							mSend(nodeType, ptr->client_socket[id]);
							mSend(nodeBrand, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.buy);
							mSend(nodeSend, ptr->client_socket[id]);

							nodeSend.Format(_T("%g"), current_node->data.sell);
							mSend(nodeSend, ptr->client_socket[id]);
						}
						break;
					}
					}
					current_node = current_node->pNext;
					if (current_node == NULL)
						break;
				} while (current_node->data.date == current_node->pNext->data.date);
				if (no_item_found == true)
					mSend(_T("0"), ptr->client_socket[id]);			//gui tin hieu la khong tim duoc du lieu
				else
					mSend(_T("-1"), ptr->client_socket[id]);		//gui tin hieu de NGUNG nhan du lieu
			}
			break;
		}
		default:
		{
			ptr->_list_server_log.AddString(client_name + _T(" vừa gửi một tín hiệu lạ."));
			break;
		}
		}
	}
	closesocket(ptr->client_socket[id]);

	return 0;
}

bool load_username_from_file(UserList& L)
{
	std::ifstream json_file_stream("username.json");
	Json::Reader json_reader;
	Json::Value json_data;

	if (!json_reader.parse(json_file_stream, json_data))
	{
		return FALSE;
	}
	for (unsigned int i = 0; i < json_data.size(); i++)
	{
		user_node* newNode = add_tail(L);
		newNode->username = json_data[i]["username"].asCString();
		newNode->password = json_data[i]["password"].asCString();
	}
	json_data.clear();
	json_file_stream.close();
	return TRUE;
}

void save_username_to_file(UserList L)
{
	std::ofstream output("username.json");
	user_node* current_node = L.pHead;

	output << "[" << std::endl;
	while (current_node != NULL)
	{
		output << "{" << std::endl;
		output << "\"username\":\"" << CStringA(current_node->username) << "\"," << std::endl;
		output << "\"password\":\"" << CStringA(current_node->password) << "\"" << std::endl;
		output << "}";
		if (current_node->pNext != NULL)
		{
			output << "," << std::endl;
			current_node = current_node->pNext;
		}
		else
		{
			output << std::endl << "]";
			break;
		}
	}
}

user_node* add_tail(UserList& L)
{
	user_node* newNode = new user_node;
	if (newNode == NULL)
		return NULL;
	if (L.pHead == NULL)
	{
		L.pHead = L.pTail = newNode;
	}
	else
	{
		L.pTail->pNext = newNode;
		L.pTail = newNode;
	}
	return newNode;
}

int mSend(CString msg, SOCKET& sock)
{
	int wstr_len = (int)wcslen(msg);	//get length
	int num_chars = WideCharToMultiByte(CP_UTF8, 0, msg, wstr_len, NULL, 0, NULL, NULL);
	CHAR* strTo = new CHAR[num_chars + 1];
	if (strTo)
	{
		WideCharToMultiByte(CP_UTF8, 0, msg, wstr_len, strTo, num_chars, NULL, NULL);
		strTo[num_chars] = '\0';
	}
	int buffSent = send(sock, (char*)&num_chars, sizeof(int), 0);
	if (buffSent <= 0)
		return 0;
	int bytesSent = send(sock, strTo, num_chars, 0);
	if (bytesSent <= 0)
		return 0;
	return bytesSent;
}

int mRecv(CString& StrRecv, SOCKET sClient)
{
	int buffLen;
	int buffReceived = recv(sClient, (char*)&buffLen, sizeof(int), 0);
	if (buffReceived <= 0)
		return buffReceived;
	char* temp = new char[buffLen + 1];
	ZeroMemory(temp, buffLen);
	int bytesReceived = recv(sClient, temp, buffLen, 0);
	temp[buffLen] = '\0';
	if (bytesReceived <= 0)
	{
		delete[]temp;
		return bytesReceived;
	}
	else
	{
		int wchar_num = MultiByteToWideChar(CP_UTF8, 0, temp, strlen(temp), NULL, 0);
		if (wchar_num <= 0)
			return -1;
		wchar_t* wstr = new wchar_t[wchar_num + 1];
		ZeroMemory(wstr, wchar_num);
		if (!wstr)
		{
			return -1;
		}
		MultiByteToWideChar(CP_UTF8, 0, temp, strlen(temp), wstr, wchar_num);
		wstr[wchar_num] = '\0';
		StrRecv = wstr;
		delete[] wstr;
		delete[] temp;
		return bytesReceived;
	}
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

int CharToWchar_t(const char* src, CString& des)
{
	int wchar_num = MultiByteToWideChar(CP_UTF8, 0, src, strlen(src), NULL, 0);
	if (wchar_num <= 0)
		return -1;
	wchar_t* wstr = new wchar_t[wchar_num + 1];
	ZeroMemory(wstr, wchar_num);
	if (!wstr)
	{
		return -1;
	}
	MultiByteToWideChar(CP_UTF8, 0, src, strlen(src), wstr, wchar_num);
	wstr[wchar_num] = '\0';
	des = wstr;
	delete[]wstr;
	return wchar_num;
}

Node* add_head(SLList& L)
{
	Node* newNode = new Node;
	if (newNode == NULL)
		return NULL;
	if (L.pHead == NULL)
		L.pHead = L.pTail = newNode;
	else
	{
		newNode->pNext = L.pHead;
		L.pHead = newNode;
	}
	return newNode;
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
	json_data.clear();
	json_file_stream.close();
	return TRUE;
}

void delete_list(UserList& L)
{
	if (L.pHead == NULL)
		return;
	user_node* current_node = L.pHead, * temp = NULL;
	while (current_node != NULL)
	{
		temp = current_node;
		current_node = current_node->pNext;
		delete temp;
	}
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

void save_data_to_file(SLList L)
{
	std::ofstream output("tygia.json");
	Node* currentNode = L.pHead;

	output << "[" << std::endl;
	while (true)
	{
		output << "{\"date\":\"" << currentNode->data.date << "\",\n\"value\":[" << std::endl;
		while (true)
		{
			output << "{" << std::endl;
			output << "\"buy\":\"" << currentNode->data.buy << "\"," << std::endl;
			output << "\"sell\":\"" << currentNode->data.sell << "\"," << std::endl;
			output << "\"company\":\"" << currentNode->data.company << "\"," << std::endl;
			output << "\"brand\":\"" << currentNode->data.brand << "\"," << std::endl;
			output << "\"type\":\"" << currentNode->data.type << "\"," << std::endl;
			output << "\"date\":\"" << currentNode->data.date << "\"" << std::endl;
			output << "}";
			if (currentNode->pNext == NULL)
				break;
			if (currentNode->data.date != currentNode->pNext->data.date)
			{
				currentNode = currentNode->pNext;
				break;
			}
			output << "," << std::endl;
			currentNode = currentNode->pNext;
		}
		output << "]" << std::endl;
		output << "}" << std::endl;

		if (currentNode->pNext == NULL)
		{
			output << "";
			break;
		}
		output << "," << std::endl;
	}
	output << "]";
	output.close();
}

size_t callback(char* buffer, size_t size, size_t num, std::string* json_data_string)
{
	size_t totalBytes = size * num;
	json_data_string->append(buffer, totalBytes);
	return totalBytes;
}

UINT update_database(LPVOID Param)
{
	CServerDlg* ptr = (CServerDlg*)Param;
	std::string url = "https://tygia.com/json.php?ran=0&rate=0&gold=1&bank=VIETCOM&date=now";
	std::string json_data_string;
	CURL* curl;

	while (true)
	{
		json_data_string.clear();
		curl = curl_easy_init();

		//Curl Option
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());					//lay url
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);		//lay ipv4
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);						//time out
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);			//viet data
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_data_string);		//gan string*out vao jsonString

		//Curl perform
		CURLcode result = curl_easy_perform(curl);
		if (result != CURLE_OK)
		{
			ptr->_list_server_log.AddString(_T("Hệ thống: Lấy dữ liệu từ web không thành công."));
			continue;
		}

		//End curl
		curl_easy_cleanup(curl);

		//Change encoding to UTF-8
		json_data_string.erase(0, 4);

		Json::Reader json_reader;
		Json::Value json_data;

		if (!json_reader.parse(json_data_string, json_data))
		{
			ptr->_list_server_log.AddString(_T("Hệ thống: Không thể dịch dữ liệu Json."));
			continue;
		}
		else
		{
			if (ptr->working_list.pHead != NULL)
			{
				int today_date = std::stoi(json_data["golds"][0]["date"].asString());

				//xóa những node lưu dữ liệu của ngày hôm nay
				Node* current_node = NULL, * temp = NULL, * pre_node = NULL;
				pre_node = ptr->working_list.pHead;
				current_node = pre_node->pNext;
				while (current_node != NULL)
				{
					if (current_node->data.date == today_date)
					{
						temp = current_node;
						current_node = current_node->pNext;
						pre_node->pNext = current_node;
						delete temp;
						continue;
					}

					pre_node = current_node;
					current_node = current_node->pNext;
				}
				ptr->working_list.pTail = pre_node;
				if (ptr->working_list.pHead->data.date == today_date)
				{
					temp = ptr->working_list.pHead;
					ptr->working_list.pHead = ptr->working_list.pHead->pNext;
					delete temp;
				}
			}

			//thêm những node dữ liệu mới
			for (unsigned int i = 0; i < json_data["golds"][0]["value"].size(); i++)
			{
				Node* newNode = add_head(ptr->working_list);

				newNode->data.buy = string_to_double(json_data["golds"][0]["value"][i]["buy"].asString());
				newNode->data.sell = string_to_double(json_data["golds"][0]["value"][i]["sell"].asString());
				newNode->data.date = std::stoi(json_data["golds"][0]["value"][i]["day"].asString());
				newNode->data.company = json_data["golds"][0]["value"][i]["company"].asString();
				newNode->data.brand = json_data["golds"][0]["value"][i]["brand"].asString();
				newNode->data.type = json_data["golds"][0]["value"][i]["type"].asString();
			}

			//Cleanup
			json_data.clear();

			//lưu dữ liệu vào file
			save_data_to_file(ptr->working_list);

		}

		ptr->_list_server_log.AddString(_T("Hệ thống: Dữ liệu của hôm nay vừa được cập nhật!"));
		Sleep(1800000);//sleep for 1 800 000 seconds = 30 minutes
	}
	return 0;
}

void CServerDlg::OnClose()
{

	// TODO: Add your message handler code here and/or call default
	closesocket(listen_socket);
	for (int i = 0; i < id; i++)
	{
		closesocket(client_socket[i]);
	}
	delete_list(working_list);
	delete_list(user_list);
	CDialogEx::OnClose();
}


