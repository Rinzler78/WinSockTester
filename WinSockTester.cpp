
#include "stdafx.h"
#include <stdlib.h> 
#include "WinSock_Server.h"

int _tmain(int argc, _TCHAR* argv[])
{
	printf("WinSock Tester\n");
	CWinSock_Server Server(7000);
	Server.Start();
	system("pause");
	return 0;
}

