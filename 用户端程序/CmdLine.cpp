#define _CRT_SECURE_NO_WARNINGS
#include "CmdLine.h"

BOOL CmdLine::cmdLine(std::string orders, std::string& result)
{
	HANDLE hRead, hWrite;
	result = "";
	SECURITY_ATTRIBUTES securityAttr;
	securityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttr.lpSecurityDescriptor = NULL;
	securityAttr.bInheritHandle = TRUE;
	if (CreatePipe(&hRead, &hWrite, &securityAttr, 0) == 0) {
		char rt[256];
		sprintf(rt, "ERROR!! CreatePipe()=[0]. GetLastError()=[%d].\n", GetLastError());
		result += rt;
		return FALSE;
	}

	STARTUPINFO            startupInfo;
	PROCESS_INFORMATION processInfo;
	DWORD dwRetVal = 0;
	startupInfo.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&startupInfo);
	startupInfo.hStdError = hWrite;            //把创建进程的标准错误输出重定向到管道输入 
	startupInfo.hStdOutput = hWrite;           //把创建进程的标准输出重定向到管道输入 
	startupInfo.wShowWindow = SW_HIDE;
	startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	char szCmdLine[2048];
	sprintf(szCmdLine, "cmd.exe /C %s", orders.c_str());
	if (CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, NULL, NULL, NULL, &startupInfo, &processInfo) == 0) {
		char rt[64];
		sprintf(rt, "ERROR!! CreateProcess()=[0]. GetLastError()=[%d].\n", GetLastError());
		result += rt;
		CloseHandle(hWrite);
		CloseHandle(hRead);
		return FALSE;
	}
	else {
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		if (GetExitCodeProcess(processInfo.hProcess, &dwRetVal) == 0) {
			char rt[64];
			sprintf(rt, "ERROR!! GetExitCodeProcess()=[0]. GetLastError()=[%d].\n", GetLastError());
			result += rt;
		}
		else {
			char rt[64];
			sprintf(rt, "The command runs successfully with returns code: [%d].\n", dwRetVal);
			result += rt;
		}
	}
	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	CloseHandle(hWrite);

	char szOutputBuffer[4096];
	DWORD dwBytesRead;
	while (true) {
		memset(szOutputBuffer, 0x00, sizeof(szOutputBuffer));
		if (ReadFile(hRead, szOutputBuffer, 4095, &dwBytesRead, NULL) == FALSE)
			break;
		char rt[5112];
		sprintf(rt, "result = [%s]\n", szOutputBuffer);
		result += rt;
	}
	CloseHandle(hRead);
	return TRUE;
}


