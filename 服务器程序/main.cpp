#include"Server.h"
#include"PublicList.h"
#include<iostream>
using namespace std;
int main() {
	SocketServer s;
	s.start(8990);
	return 0;
}