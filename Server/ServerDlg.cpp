
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
UINT new_client(LPVOID param);

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
	//	ON_WM_CLOSE()
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

		ptr->client_addr_len[ptr->id + 1] = sizeof(ptr->client_addr[ptr->id + 1]);
		ptr->client_socket[ptr->id + 1] = accept(ptr->listen_socket,
			(sockaddr*)&ptr->client_addr[ptr->id + 1], &ptr->client_addr_len[ptr->id + 1]);
		ptr->id++;
		ptr->client_thread[ptr->id] = AfxBeginThread(new_client, ptr);
	}

	return 0;
}

UINT new_client(LPVOID param)
{
	CServerDlg* ptr = (CServerDlg*)param;
	int id = ptr->id;
	CString _t_id;
	_t_id.Format(_T("%d"), id);
	CString msg = _T("Client ") + _t_id + _T(" đã kết nối.");
	ptr->_list_server_log.AddString(msg);
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

	output << "[";
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

size_t callback(char* buffer, size_t size, size_t num, std::string* json_data)
{
	size_t totalBytes = size * num;
	json_data->append(buffer, totalBytes);
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

		int today_date = 0;
		Json::Reader json_reader;
		Json::Value json_data;

		if (!json_reader.parse(json_data_string, json_data))
		{
			ptr->_list_server_log.AddString(_T("Hệ thống: Không thể dịch dữ liệu Json."));
			continue;
		}
		else
		{
			today_date = std::stoi(json_data["golds"][0]["date"].asString());

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

			//thêm những node dữ liệu mới
			for (int i = 0; i < json_data["golds"][0]["value"].size(); i++)
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


		ptr->_list_server_log.AddString(_T("Dữ liệu của hôm nay vừa được cập nhật!"));
		Sleep(1800000);//sleep for 1 800 000 seconds = 30 minutes
	}
	return 0;
}

void CServerDlg::OnBnClickedBtnShutdown()
{
	// TODO: Add your control notification handler code here

}

void CServerDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	delete_list(working_list);
	CDialogEx::OnClose();
}
