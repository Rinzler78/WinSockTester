#pragma once
#include "Winsock2.h"
#include "WinSock_ServerClient.h"
#pragma comment(lib,"Ws2_32.lib")

typedef enum ActionIDS
{
	ActionIDS_EXIT = 0,
	ActionIDS_CleanClientList,
	ActionIDS_PurgeCLientList,
	MAX_ActionIDS
}ActionIDS_t;

const char ActionIDS_STR_TAB [MAX_ActionIDS][30]
={"Exit Request", "Clean Client List Request", "Purge Client List Request"};

class CWinSock_ServerClient;
class CLinkedList;

class CWinSock_Server : public CI_WinSock_ServerClient_Delegate
{
public:
	CWinSock_Server(unsigned int ListeningPortNumber, unsigned int BackLog = 50, int Familly = AF_INET, int Type = SOCK_STREAM, int Protocole = IPPROTO_TCP);
	~CWinSock_Server(void);

	bool Start();
	bool Stop();

private:
	void ClientDisconnected();
	void CleanClientList();/** Delete All client */
	void PurgeClientList();/** Delete Disconnected Client */
	
	unsigned int m_ListeningPortNumber;
	SOCKET m_ListeningSocket;
	int m_Familly;
	int m_Type;
	int m_Protocole;
	unsigned int m_BackLog;
	bool m_IsListening;
	struct sockaddr_in m_saServer;

	/** Accept Thread */
	friend DWORD AccepteRoutine(LPVOID lpdwThreadParam );
	HANDLE m_hAcceptThread;
	DWORD m_AcceptThreadID;
	CLinkedList * m_pClientList;

	/** Command Thread */
	friend DWORD ActionThreadRoutine(LPVOID lpdwThreadParam);
	HANDLE m_CommandThread;
	DWORD m_CommandThreadID;
	HANDLE m_CommandEventTab [MAX_ActionIDS];
};

