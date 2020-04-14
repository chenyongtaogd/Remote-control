#pragma once
class SocketServer
{
public:SocketServer()
{
	
}
public:	~SocketServer() 
{
		
}
public:
	int start(int port);
};
class Server:public SocketServer
{
	Server() 
	{

	}
	
	~Server()
	{

	}
};

