#include<direct.h>
#include<io.h>
#include<iostream>
#include<Windows.h>
using namespace std;
#define SELFSTART_REGEDIT_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\"
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) //隐藏窗口
int main()
{
	/*结束进程*/
	system("taskkill /f /t /im IntelCpHDCPSvc.exe");
	/*删除文件*/
	HKEY hKey = NULL;
	LONG lRet = NULL;
	int flag = 0;
	for (int i = 0; i < 6; i++)
	{
		string name = "";
		string name1 = "";
		string name2 = "";
		char dir[4] = { 'C' + i,':','\\','\0' };
		name += dir;
		name1 += dir;
		name2 += dir;
		name += "m00zz12548dqqfgtad7895521\\a00zz12548dqqfgtad7\\IntelCpHDCPSvc.exe";
		name1 += "m00zz12548dqqfgtad7895521\\a00zz12548dqqfgtad7";
		name2 += "m00zz12548dqqfgtad7895521";
		if (_access(name.c_str(), 0) == 0)
		{
			remove(name.c_str());
			_rmdir(name1.c_str());
			SetFileAttributes(name2.c_str(), FILE_ATTRIBUTE_NORMAL);
			_rmdir(name2.c_str());
			lRet = RegOpenKeyExA(HKEY_CURRENT_USER, SELFSTART_REGEDIT_PATH, 0, KEY_ALL_ACCESS, &hKey);
			//判断是否成功
			if (lRet != ERROR_SUCCESS)
			{
			}
			else
			{
				//删除名为"WinsysRun"的注册表信息
				RegDeleteValueA(hKey, "WinsysRun");
				//关闭注册表            
				RegCloseKey(hKey);
			}
			MessageBox(NULL, "用户端已清除", "运行成功", MB_OK | MB_ICONASTERISK);
		}
		else
			flag++;
	} 
	if(flag==6)
		MessageBox(NULL, "未发现残留数据", "tips", MB_OK | MB_ICONASTERISK);

}