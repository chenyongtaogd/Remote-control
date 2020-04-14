#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Server.h"
#include"winsock2.h"
#pragma comment(lib,"ws2_32.lib")
#include<iostream>
#include"PublicList.h"
using namespace std;

/*
		数据包结构
		1.数据头 6652：断开连接 6653：申请建立连接 6654：同意连接
		2.数据包
		2.1命令
		2.2文件头
		2.3文件
*/
typedef struct transBag {
	int transBagHead;
	union {
		char num[13];
		char transBagOrder[1024];
		struct 
		{
			char name[1024];
			long length;
		}transBagFileHead;
		char transBagFileBody[2048];
	}transBagBody;

}transBag;


/*
ThreadforSeverRecv(LPVOID lParameter)
服务端接收消息用的线程函数，传入参数NULL
单处理，单转发
确保数据包发送顺序不变
*/
DWORD WINAPI ThreadforSeverRecv(LPVOID lParameter)
{
	
	SOCKET* c = (SOCKET*)(lParameter);
	SOCKET clientSocket = *c;
	SOCKET toSocket = 0;
	/*判断是用户端还是控制端，0是用户端，1是控制端*/
	int flag = 0;
	while (1) 
	{
		transBag transBagRecv;
		int r = recv(clientSocket, (char*)& transBagRecv, sizeof(transBag), 0);
		if (r > 0)
		{
			switch (transBagRecv.transBagHead)
			{
			//case 1://指令
			//	break;
			//case 2://文件信息
			//	break;
			//case 3://文件主体
			//	break;
			case 4://用户登陆
				/*
				1.加入在线用户组
				*/
				cout << "新的用户组加入：" << transBagRecv.transBagBody.num << endl;
				UserMsg userMsg;
				for(int i=0;i<13;i++)
					userMsg.num[i] = transBagRecv.transBagBody.num[i];
				userMsg.num[12] = '\0';
				userMsg.socketUser = clientSocket;
				userMsg.socketCtrl = 0;
				for (int i = 0; i < userList.size(); i++)
				{
					if (strcmp(userList[i].num, userMsg.num) == 0)
					{
						userList.erase(userList.begin() + i);
						break;
					}
				}
				userList.push_back(userMsg);
				break;
			case 5://控制登陆
				/*
				1.加入控制组
				*/
				cout << "新的控制组加入："<< transBagRecv.transBagBody.num << endl;
				CtrlMsg ctrlMsg;
				for (int i = 0; i < 13; i++)
					ctrlMsg.num[i] = transBagRecv.transBagBody.num[i];
				ctrlMsg.num[12] = '\0';
				ctrlMsg.socketCtrl = clientSocket;
				ctrlMsg.socketUser = 0;
				for (int i = 0; i < ctrlList.size(); i++)
				{
					if (strcmp(ctrlList[i].num, ctrlMsg.num) == 0)
					{
						ctrlList.erase(ctrlList.begin() + i);
						break;
					}
				}
				ctrlList.push_back(ctrlMsg);
				/*
				发送用户组
				*/
				
				for (int i = 0; i < userList.size(); i++)
				{
					struct UseableNum
					{
						char num[13];
						bool useable;
					}useableNum;
					for (int m = 0; m < 13; m++)
					{
						useableNum.num[m] = userList[i].num[m];
					}
					if (userList[i].socketCtrl == 0)
						useableNum.useable = true;
					else
						useableNum.useable = false;

					send(clientSocket, (char*)&useableNum, sizeof(UseableNum), 0);
				}
				/*
				发送结束符
				*/
				send(clientSocket, "#############", 14, 0);
				flag = 1;
				break;
			case 6654://同意连接
				for (int i = 0; i < userList.size(); i++)
				{
					if (userList[i].socketUser==clientSocket)
					{
						toSocket = userList[i].socketCtrl;
						break;
					}
				}
				if (toSocket)
				{
					
					transBag transBagSend;
					memset(&transBagSend, 0, sizeof(transBag));
					transBagSend.transBagHead = 6654;
					send(toSocket, (char*)& transBagSend, sizeof(transBag), 0);
				}
				break;
			case 6653:
				/*
				控制端发起连接
				1.控制端像服务器发送信息
				2.服务器（的控制端）保存用户socket，更新控制组,服务器（的用户端）保存控制端socket，更新用户组
				3.服务器向用户端发送信息
				4.用户端返回同意连接或连接失败信息
				5.建立连接，向控制端发送信息
				6.或连接失败，向控制端发送信息
				*/
				toSocket = 0;
				for (int i = 0; i < userList.size(); i++)
				{
					if (strcmp(userList[i].num, transBagRecv.transBagBody.num)==0)
					{
						toSocket = userList[i].socketUser;
						break;
					}
				}
				if (toSocket)
				{
					
					for (int i = 0; i < ctrlList.size(); i++)
					{
						if (ctrlList[i].socketCtrl == clientSocket)
						{
							ctrlList[i].socketUser = toSocket;
							break;
						}
					}
					for (int i = 0; i < userList.size(); i++)
					{
						if (userList[i].socketUser == toSocket)
						{
							userList[i].socketCtrl = clientSocket;
							break;
						}
					}
					transBag transBagSend;
					memset(&transBagSend, 0, sizeof(transBag));
					transBagSend.transBagHead = 6653;
					send(toSocket, (char*)& transBagSend, sizeof(transBag), 0);
				}
				else
				{
					/*用户瞬间下线*/
					transBag transBagSend;
					memset(&transBagSend, 0, sizeof(transBag));
					transBagSend.transBagHead = 6652;
					send(clientSocket, (char*)& transBagSend, sizeof(transBag), 0);
				}
				break;
			case 2:
			{
#define LENGTH_FILE_BODY 2048
				if (toSocket)
				{
					send(toSocket, (char*)& transBagRecv, sizeof(transBag), 0);
				}
				char szBuf[LENGTH_FILE_BODY] = { 0 };
				int len = 0;
				long flen = transBagRecv.transBagBody.transBagFileHead.length;
				while (flen)
				{
					len = recv(clientSocket, szBuf, LENGTH_FILE_BODY, 0);
					if (len > 0)
					{
						if (toSocket)
						{
							send(toSocket, szBuf, len, 0);
							flen -= len;
						}
					}
					else if (len <= 0 && errno != EINTR)
						break;
				}
			}
				break;
			default:
				if (toSocket)
				{
					send(toSocket, (char*)& transBagRecv, sizeof(transBag), 0);
				}
			}
		}
		else if (r <= 0 && errno != EINTR)
		{
			/*
			用户断开连接
			1.从用户组删除
			2.通知控制	控制端判断当前任务，清除正在进行的任务
			3.退出循环
			控制断开连接
			1.从控制组删除
			2.通知用户	用户判断当前任务，清除正在进行的任务
			3.用户组的重置
			4.退出循环
			*/

			if (!flag)
			{/*用户端*/
				
				for (int i = 0; i < userList.size(); i++)
				{
					if (userList[i].socketUser == clientSocket)
					{
						cout << "用户下线：" << userList[i].num<< endl;
						if(!toSocket)
						toSocket = userList[i].socketCtrl;
						userList.erase(userList.begin() + i);
						break;
					}
				}

				if (toSocket)
				{
					transBag transBagSend;
					memset(&transBagSend, 0, sizeof(transBag));
					transBagSend.transBagHead = 6652;
					send(toSocket, (char*)& transBagSend, sizeof(transBag), 0);
					
				}

				closesocket(clientSocket);
				return 0;
			}
			else 
			{/*控制端*/
				for (int i = 0; i < ctrlList.size(); i++)
				{
					if (ctrlList[i].socketUser == clientSocket)
					{
						cout << "控制端下线:" << ctrlList[i].num << endl;
						if (!toSocket)
							toSocket = ctrlList[i].socketUser;
						ctrlList.erase(ctrlList.begin() + i);
						break;
					}
				}

				if (toSocket)
				{
					transBag transBagSend;
					memset(&transBagSend, 0, sizeof(transBag));
					transBagSend.transBagHead = 6652;
					send(toSocket, (char*)& transBagSend, sizeof(transBag), 0);
					/*for (int i = 0; i < userList.size(); i++)
					{
						if (userList[i].socketUser == toSocket)
						{
							userList[i].socketUser = 0;
							break;
						}
					}*/
				}

				closesocket(clientSocket);
				return 0;
			}
			break;
		}
	}


	/*
	1.关闭socket
	*/
	closesocket(clientSocket);
	return 0;
}

/*
start(int port),创建初始化套接字，开放port端口
*/
int SocketServer::start(int port) 
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return ERROR;
	SOCKET severSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (severSocket == INVALID_SOCKET)
		return ERROR;
	if (!port)
		return ERROR;
	SOCKADDR_IN severAddr;
	severAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	severAddr.sin_family = AF_INET;
	severAddr.sin_port = htons(port);

	if (-1 == bind(severSocket, (struct sockaddr*) & severAddr, sizeof(struct sockaddr)))
		return ERROR;

	if (-1 == listen(severSocket, 5))
		return ERROR;

	int len = sizeof(sockaddr);
	long long num = 0;
	while (1)
	{

		SOCKADDR_IN clientAddr;
		SOCKET clientSocket = accept(severSocket, (sockaddr*)& clientAddr, &len);
		/*
		创建线程->
		1.接收数据->判断特征信息->存入链表
		2.接收数据->判断发送目标->发送数据
		3.接收数据->判断连接状态->存入链表
		创建线程
		1.一定时间->导出链表信息->写入文件
		*/
		
		cout << "新连接  ";
		printf("Accept client IP:[%s]   ", inet_ntoa(clientAddr.sin_addr));
		printf("port:[%d]", clientAddr.sin_port);
		cout << endl;
		HANDLE handle = CreateThread(NULL, 0, ThreadforSeverRecv, &clientSocket, 0, NULL);
		CloseHandle(handle);
	}
}