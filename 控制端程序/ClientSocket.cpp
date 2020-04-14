#include "ClientSocket.h"
#include"GetMAC.h"
#include <winsock.h>
#include <fstream>
#include<iostream>
#include<io.h>
#include<direct.h>
#include<vector>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

/*全局变量*/
namespace PublicMumber {
	SOCKET socket;
	int if_break;//1：中断操作，0：正常
	string path_download;
	string name_file;
	string path_now;
	string root_now;
	vector<string> list_num;
};



/*
		数据包结构
		1.数据头 6652：断开连接 6653：申请建立连接 6654：同意连接 6655:文件发送成功 6656:下载文件 6657:终止下载
				 6658:文件不存在 6659:文件可以下载 6660:查看目录是否存在 6661：存在 6662：不存在
				 6663:删除文件/文件夹  6664：删除成功  6665：删除失败

		2.数据包
		2.1命令
		2.2文件头
		2.3文件
*/
typedef struct transBag {
	int transBagHead;
	union {
		char num[13];
		char transBagOrder[2048];
		struct
		{
			char name[1024];
			long length;
		}transBagFileHead;
		char transBagFileBody[2048];
	}transBagBody;

}transBag;



ClientSocket::ClientSocket()
{
}

ClientSocket::~ClientSocket()
{
}

/*重要函数*/
int ClientSocket::start(int port, char* ip)

{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return ERROR;
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return ERROR;
	SOCKADDR_IN clientAddr;
	clientAddr.sin_addr.S_un.S_addr = inet_addr(ip);
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(port);
	
	if (connect(clientSocket, (struct  sockaddr*) & clientAddr, sizeof(clientAddr)) == INVALID_SOCKET)
	{
		cout << "网络异常" << endl;
		return ERROR;
	}
		
	/*
	1.连接成功
	2.获取本机标识码
	3.发送本机标识码
	*/
	std::string macAdress;
	if (!GetMAC::GetMacByGetAdaptersInfo(macAdress))
		return ERROR;

	transBag transBagSendLogin;
	memset(&transBagSendLogin, 0, sizeof(transBag));
	transBagSendLogin.transBagHead = 5;
	for (int i = 0; i < macAdress.size(); i++)
		transBagSendLogin.transBagBody.num[i] = macAdress[i];
	send(clientSocket, (char*)& transBagSendLogin, sizeof(transBag), 0);
	struct UseableNum
	{
		char num[13];
		bool useable;
	}useableNum;
	int number = 0;
	while (1)
	{
		int r = recv(clientSocket, (char*)& useableNum, sizeof(UseableNum), 0);
		if (r > 0)
		{
			if (strcmp(useableNum.num, "#############") == 0)
			{
				break;
			}
			cout << "[" << number << "] " << useableNum.num;
			if (useableNum.useable == true)
				cout << "      free" << endl;
			else
				cout << "    connected" << endl;
			PublicMumber::list_num.push_back(useableNum.num);
			number++;
		}
		else
		{
			cout << "服务器连接异常" << endl;
			return 0;
		}
	}
	if (PublicMumber::list_num.size() == 0)
	{
		cout << "无可用用户" << endl;
		closesocket(clientSocket);
		return 0;
	}
	{
		int numberChoose = 0;
		cin >> numberChoose;
		transBag transBagSendConnect;
		memset(&transBagSendConnect, 0, sizeof(transBag));
		transBagSendConnect.transBagHead = 6653;
		for (int i = 0; i < PublicMumber::list_num[numberChoose].size(); i++)
		{
			transBagSendConnect.transBagBody.num[i] = PublicMumber::list_num[numberChoose][i];
		}
		send(clientSocket, (char*)& transBagSendConnect, sizeof(transBag), 0);
		transBag transBagRecv;
		memset(&transBagRecv, 0, sizeof(transBag));
		int r = recv(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
		if (r > 0)
		{
			switch (transBagRecv.transBagHead)
			{
			case 6652:
				cout << "连接失败" << endl;
				break;
			case 6654:
				cout << "连接成功" << endl;
				//cout << PublicMumber::root_now << PublicMumber::path_now << ">";
				break;
			}
		}
	}
	
	PublicMumber::socket = clientSocket;

	Input::start();
	while (1)
	{
		transBag transBagRecv;
		memset(&transBagRecv, 0, sizeof(transBag));
		transBag transBagSend;
		int r = recv(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
		if (r > 0)
		{
			std::string result = "";
			memset(&transBagSend, 0, sizeof(transBag));

			switch (transBagRecv.transBagHead)
			{
			case 1://输出
			{
				cout << transBagRecv.transBagBody.transBagOrder;
			}
				break;
			case 2://文件信息
			{
#define LENGTH_FILE_BODY 2048
				std::fstream out;//写入文件
				out.open(PublicMumber::path_download+"\\"+PublicMumber::name_file,
					std::fstream::out | std::fstream::binary | std::fstream::trunc);
				long flen = transBagRecv.transBagBody.transBagFileHead.length;
				int len = 0;
				int flag = 0;//是否下载成功
				PublicMumber::if_break = 0;
				cout << "文件：		" << PublicMumber::name_file << endl;
				cout << "大小：		" << flen << " B		" << endl;
				cout << "下载中：	";
				cout << "   ";
				long Flen = flen;
				char content[2048] = { 0 };
				while (flen)
				{
					len = recv(clientSocket, content, LENGTH_FILE_BODY, 0);
					if (len > 0)
					{
						out.write(content, len);
						flen -= len;
						if (flen == 0)flag = 1;
					}
					else if (len <= 0 && errno != EINTR)
						break;

					if ((int)(Flen - flen) * 100 / (int)Flen < 10 && (int)(Flen - flen) * 100 / (int)Flen >= 0)
						cout << "\b\b";
					else
					if ((int)(Flen - flen) * 100 / (int)Flen >= 10 && (int)(Flen - flen) * 100 / (int)Flen <= 100)
						cout << "\b\b\b";
					cout << (int)(Flen - flen) * 100 / (int)Flen << "%";
				}
				out.close();
				if (flag)
				{
					cout << "\n" << PublicMumber::name_file << " 下载成功" << endl;
				}
			}
				break;
			case 6652://断开连接
			{
				cout << "\n断开连接" << endl;
				closesocket(PublicMumber::socket);
				WSACleanup();
				return 0;
			}	
				break;
			case 6654://连接成功
			{
				cout << "连接成功" << endl;
			}
				break;
			case 6655://上传成功
			{
				cout << "\n文件上传成功" << endl;
			}
				break;
			case 6658://文件下载失败
			{
				cout << "文件下载失败" << endl;
			}
				break;
			case 6661:///路径存在
			{/*路径存在*/
				PublicMumber::root_now = "";
				PublicMumber::path_now = "";
				for (int i = 0; i < 2048; i++)
				{
					if (transBagRecv.transBagBody.transBagOrder[i] == '\0')break;
					if (i < 3)
						PublicMumber::root_now.push_back(transBagRecv.transBagBody.transBagOrder[i]);
					else
						PublicMumber::path_now.push_back(transBagRecv.transBagBody.transBagOrder[i]);
				}
				if(PublicMumber::root_now.size()==2)PublicMumber::root_now.push_back('\\');
			}
				break;
			case 6662://目录不存在
			{
				cout << "目录不存在" << endl;
			}
				break;
			case 6664://删除成功
			{
				cout << "删除成功" << endl;
			}
				break;
			case 6665://删除失败
			{
				cout << "删除失败" << endl;
			}
				break;
			default:
				break;
			}
			cout << PublicMumber::root_now << PublicMumber::path_now << ">";
		}
		else if (r <= 0 && errno != EINTR)
		{
			closesocket(PublicMumber::socket);
			WSACleanup();
			break;
		}
	}
	return 1;
}

/*Input类*/
Input::Input()
{
}
Input::~Input()
{
}

/*重要函数*/
DWORD WINAPI InputThread(LPVOID lpParameter)
{
	SOCKET clientSocket = PublicMumber::socket;
	transBag transBagSend;
	transBag transBagRecv;
	memset(&transBagRecv, 0, sizeof(transBag));
	while (1)
	{
		/*cout << PublicMumber::root_now << PublicMumber::path_now << ">";*/
		char orders[2048];
		cin.getline(orders, 2048);
		string orders_s = orders;
		string orderFirst = "";
		for (int i = 0; i < 2048; i++)
		{
			if (orders[i] == ' ' || orders[i] == '\0')
			{
				break;
			}
			orderFirst.push_back(orders[i]);
		}
		if (orderFirst == "dir")
		{
			/*dir + path_now*/
			string sendOrders = "";
			if (orders_s.size() == orderFirst.size())
			{
				sendOrders += "dir ";
				sendOrders += PublicMumber::root_now;
				sendOrders += PublicMumber::path_now;
			}
			else
			{
				sendOrders = orders_s;
			}
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 1;
			int i = 0;
			for (; i < sendOrders.size(); i++)
			{
				transBagSend.transBagBody.transBagOrder[i] = sendOrders[i];
			}
			transBagSend.transBagBody.transBagOrder[i] = '\0';
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
		}
		else if (orderFirst == "cd")//change dir
		{
			/*查看目录，存在则改变，反之不变*/
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 6660;
			string path;
			string path_now_change;
			GetSecondOrder(orders, path);
			if (PublicMumber::path_now.size() != 0)
				path_now_change = PublicMumber::path_now + "\\" + path;
			else
				path_now_change = PublicMumber::path_now + path;
			string patn_all = PublicMumber::root_now + path_now_change;
			int i_pnc = 0;
			for (; i_pnc < patn_all.size(); i_pnc++)
			{
				transBagSend.transBagBody.transBagOrder[i_pnc] = patn_all[i_pnc];
			}
			transBagSend.transBagBody.transBagOrder[i_pnc] = '\0';
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
			
		}
		else if (orderFirst == "cd..")
		{
			/*查看目录，最后一层则不变，反之后退一层*/
			int end = PublicMumber::path_now.size();

			for (int i = PublicMumber::path_now.size() - 1; i >= 0; i--)
			{
				if (PublicMumber::path_now[i] == '\\' || i == 0)
				{
					end = i;
					break;
				}
			}
			string path_now_change = "";
			for (int i = 0; i < end; i++)
			{
				path_now_change.push_back(PublicMumber::path_now[i]);
			}
			PublicMumber::path_now = path_now_change;
			cout << PublicMumber::root_now << PublicMumber::path_now << ">";
		}
		else if (orders_s.size() == 2 && orders_s[1] == ':')
		{
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 6660;
			int i_root = 0;
			for (; i_root < orders_s.size(); i_root++)
			{
				transBagSend.transBagBody.transBagOrder[i_root] = orders_s[i_root];
			}
			transBagSend.transBagBody.transBagOrder[i_root] = '\0';
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
			//recv(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
			//if (transBagRecv.transBagHead == 6661)
			//{
			//	PublicMumber::path_now = orders_s + "\\";
			//}
			//else if (transBagRecv.transBagHead == 6662)
			//{
			//	/*什么也不做*/
			//}
		}
		else if (orderFirst == "upload")
		{
			/*上传文件*/
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 2;
			string nameWithPath = PublicMumber::root_now + PublicMumber::path_now;
			string secondOrders;
			string name="";
			GetSecondOrder(orders, secondOrders);
			fstream fs(secondOrders, fstream::in | fstream::binary);
			if (!fs.is_open()) {
				cout << "文件不存在" << endl;
				cout << PublicMumber::root_now << PublicMumber::path_now << ">";
				continue;
			}
			
			int last_line = 0;
			for (int i = secondOrders.size() - 1; i >= 0; i--)
			{
				if (secondOrders[i] == '\\')
				{
					last_line = i;
					break;
				}
			}
			for (int i = last_line; i < secondOrders.size(); i++)
			{
				nameWithPath.push_back(secondOrders[i]);
				if (i != last_line)
					name.push_back(secondOrders[i]);
			}
			fs.seekg(0, fstream::end);//以最后的位置为基准不偏移
			long Flen = fs.tellg();//取得文件大小
			fs.seekg(0, fstream::beg);
			int i_name = 0;
			for (; i_name < nameWithPath.size(); i_name++)
			{
				transBagSend.transBagBody.transBagFileHead.name[i_name] = nameWithPath[i_name];
			}
			transBagSend.transBagBody.transBagFileHead.name[i_name] = '\0';
			transBagSend.transBagBody.transBagFileHead.length = Flen;
			int r = send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
			if (r <= 0)
			{
				fs.close();
				continue;
			}
			cout << "文件：		" << name << endl;
			cout << "大小：		" << Flen << " B		" << endl;
			cout << "上传中：	";
			cout << "   ";
			long flen = 0;
#define LENGTH_FILE_BODY 2048
			char szBuf[LENGTH_FILE_BODY] = { 0 };
			while (!fs.eof())
			{
				fs.read(szBuf, LENGTH_FILE_BODY);
				int len = fs.gcount();
				flen += send(clientSocket, szBuf, len, 0);
				if ((int)flen * 100 / (int)Flen < 10 && (int)flen * 100 / (int)Flen >= 0)
					cout << "\b\b";
				else
					if ((int)flen * 100 / (int)Flen >= 10 && (int)flen * 100 / (int)Flen <= 100)
						cout << "\b\b\b";
				cout << (int)flen * 100 / (int)Flen << "%";
			}
		}
		else if (orderFirst == "download")
		{
			/*发送下载指令，去掉download，直接发送文件名，6656*/
			string sendOrders="";
			string secondOrders;
			GetSecondOrder(orders, secondOrders);
			sendOrders = PublicMumber::root_now + PublicMumber::path_now + "\\" + secondOrders;
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 6656;
			int i = 0;
			for (; i < sendOrders.size(); i++)
			{
				transBagSend.transBagBody.transBagOrder[i] = sendOrders[i];
			}
			transBagSend.transBagBody.transBagOrder[i] = '\0';
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
			PublicMumber::name_file = secondOrders;
		}
		else if (orderFirst == "start")
		{
			/*打开文件*/
			string sendOrders = "";
			string secondOrders;
			GetSecondOrder(orders, secondOrders);
			sendOrders = "start " + PublicMumber::root_now + PublicMumber::path_now + "\\" + secondOrders;
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 1;
			int i = 0;
			for (; i < sendOrders.size(); i++)
			{
				transBagSend.transBagBody.transBagOrder[i] = sendOrders[i];
			}
			transBagSend.transBagBody.transBagOrder[i] = '\0';
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
		}
		else if (orderFirst == "rm")
		{
		memset(&transBagSend, 0, sizeof(transBag));
		transBagSend.transBagHead = 6663;
		string name;
		string path_all;
		GetSecondOrder(orders, name);
		path_all = PublicMumber::root_now+ PublicMumber::path_now + "\\" + name;
		int i_pnc = 0;
		for (; i_pnc < path_all.size(); i_pnc++)
		{
			transBagSend.transBagBody.transBagOrder[i_pnc] = path_all[i_pnc];
		}
		transBagSend.transBagBody.transBagOrder[i_pnc] = '\0';
		send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
		}
		else if (orderFirst == "cdd")//change download dir
		{
			/*改变下载地址*/
			string secondOrders;
			int rt = 0;
			GetSecondOrder(orders, secondOrders);
			if (0 != _access(secondOrders.c_str(), 0))
			{
				// if this folder not exist, create a new one.
				rt = _mkdir(secondOrders.c_str());				// 返回 0 表示创建成功，-1 表示失败
			}
			if (rt == 0)
				PublicMumber::path_download = secondOrders;
			else
				cout << "未能改变下载目录，目录存在非法字符" << endl;
		}
		else if (orderFirst == "break")
		{//退出程序
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 6666;
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
			cout << "用户端已关闭" << endl;
		}
		else if (orderFirst == "clear")
		{
			system("cls");
			cout << PublicMumber::root_now << PublicMumber::path_now << ">";
		}
		else
		{
			/*其他指令，直接发送，空指令continue*/
			string sendOrders =orders;
			if (sendOrders.size() == 0)
			{
				cout << PublicMumber::root_now << PublicMumber::path_now << ">";
				continue;
			}
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 1;
			int i = 0;
			for (; i < sendOrders.size(); i++)
			{
				transBagSend.transBagBody.transBagOrder[i] = sendOrders[i];
			}
			transBagSend.transBagBody.transBagOrder[i] = '\0';
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);


		}
	}
	return 0;
}

int Input::start()
{
	/*
	1.创建线程
	2.输入->输入处理
	2.1->发送
	2.2->本地指令  2.2.1->清空  2.2.2cd  2.2.3 改变下载目录
	2.3->上传文件
	*/
	CreateThread(NULL, 0, InputThread, NULL, 0, NULL);
	return 0;
}

/*搜索指令的第二部分*/
void GetSecondOrder(string orders,string &secondOrder)
{
	secondOrder = "";
	int i = 0;
	for (; i < orders.size(); i++)
	{
		if (orders[i] != ' ')break;
	}
	for ( ;i < orders.size(); i++)
	{
		if (orders[i] == ' ')break;
	}
	for (; i < orders.size(); i++)
	{
		if (orders[i] != ' ')break;
	}
	for (; i < orders.size(); i++)
	{
		secondOrder.push_back(orders[i]);
	}
	/*for (i = 0; i < SecondOrder.size(); i++)
	{
		secondOrder[i] = SecondOrder[i];
	}
	secondOrder[i] = '\0';*/
}

/*初始化公共变量*/
void InitPublicMumber()
{
	PublicMumber::socket=0;
	PublicMumber::if_break=0;//1：中断操作，0：正常
	PublicMumber::path_download="C:\\download";
	PublicMumber::name_file="";
	PublicMumber::path_now = "abc";//"Users\\win10\\Desktop";
	PublicMumber::root_now="C:\\";
	PublicMumber::list_num;
	if (0 != _access(PublicMumber::path_download.c_str(), 0))
	{
		// if this folder not exist, create a new one.
		_mkdir(PublicMumber::path_download.c_str());				// 返回 0 表示创建成功，-1 表示失败
	}
}