#pragma once
class ClientSocket
{
public:
	ClientSocket();
	~ClientSocket();

	int start(int port, char* ip);
};

