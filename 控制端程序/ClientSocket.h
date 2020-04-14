#pragma once
#include<string>
class ClientSocket
{
public:
	ClientSocket();
	~ClientSocket();

	int start(int port, char* ip);
};
/*输入类*/
class Input
{
public:
	Input();
	~Input();

	static int start();
};


/*搜索指令的第二部分*/
void GetSecondOrder(std::string orders, std::string &secondOrder);
/*初始化公共变量*/
void InitPublicMumber();