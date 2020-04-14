#pragma once
#include <string>
#include <process.h>
#include <string>
#include <windows.h>

class CmdLine
{
public:
	static BOOL cmdLine(std::string orders, std::string& result);
};

