#include "WinSock_Server.h"
#include "stdio.h"
#include "WinSock_ServerClient.h"
#include "LinkedList.h"
#define PrintWSAError printf("error code %u\n", WSAGetLastError());

CWinSock_Server::CWinSock_Server(unsigned int ListeningPortNumber, unsigned int BackLog, int Familly, int Type, int Protocole)
:m_ListeningPortNumber(ListeningPortNumber)
, m_BackLog(BackLog)
, m_ListeningSocket(NULL)
, m_Familly(Familly)
, m_Type(Type)
, m_Protocole(Protocole)
, m_IsListening(false)
, m_hAcceptThread(NULL)
, m_AcceptThreadID(0)
, m_pClientList(new CLinkedList())
, m_CommandThread(NULL)
{
	WSADATA wsaData = {0};

	/** Start WinSock DLL */
	if(WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("WSA Startup Failure\n");
		PrintWSAError
		return;
	}
	
	/** Init Action Events */
	memset(&m_CommandEventTab, 0, sizeof(m_CommandEventTab));

	for(unsigned int i = 0 ; i < MAX_ActionIDS ; ++i)
	{
		m_CommandEventTab[i] = CreateEvent(NULL, false, false, NULL);
	}

	/** Init Action thread */
	m_CommandThread = CreateThread(NULL, 128, (LPTHREAD_START_ROUTINE)&ActionThreadRoutine, this, 0, &m_CommandThreadID);
}
CWinSock_Server::~CWinSock_Server(void)
{
	Stop();

	/** Wait for Action thread Finish */
	SetEvent(m_CommandEventTab[ActionIDS_EXIT]);
	WaitForSingleObject(m_CommandThread, INFINITE);

	/** Clean Client List */
	CleanClientList();

	/** Delete Client List */
	if(m_pClientList)
		delete m_pClientList;

	WSACleanup();
}

bool CWinSock_Server::Start()
{
	if(m_IsListening)
		return false;

	int iResult = 0;
	hostent* plocalHost = NULL;
	char* plocalIP = NULL;

	/** Create The Socket */
	m_ListeningSocket = socket(m_Familly, m_Type, m_Protocole);
	if(m_ListeningPortNumber == INVALID_SOCKET)
	{
		printf("Socket Creation Failure\n");
		PrintWSAError
		return false;
	}

	memset(&m_saServer, 0, sizeof(m_saServer));
	// Get the local host information
	if(!(plocalHost = gethostbyname("")))
	{
		printf("Unable to retrieve host by adresse\n");
		return false;
	}
	printf("Host Name : %s\n", plocalHost->h_name);
	if(!(plocalIP = inet_ntoa (*(struct in_addr *)*plocalHost->h_addr_list)))
	{
		printf("Unable to ip from list\n");
		return false;
	}
	printf("Host Ip : %s\n", plocalIP);

	// Set up the sockaddr structure
	m_saServer.sin_family = AF_INET;
	m_saServer.sin_addr.s_addr = inet_addr(plocalIP);
	m_saServer.sin_port = htons(m_ListeningPortNumber);

	/** Binding */
	if(bind( m_ListeningSocket,(SOCKADDR*) &m_saServer, sizeof(m_saServer) ))
	{
		printf("Binding on %s:%u failure\n", plocalIP, m_ListeningPortNumber);
		PrintWSAError
		return false;
	}
	printf("Binding Success\n");

	/** Start Listening */
	if(listen(m_ListeningSocket, m_BackLog))
	{
		printf("Start Listening on port %u failure\n", m_ListeningPortNumber);
		PrintWSAError
		return false;
	}
	printf("Listening on %u start with %u Backlog length\n", m_ListeningPortNumber, m_BackLog);

	/** Launching Acceptation thread */
	m_hAcceptThread = CreateThread(NULL, 128, (LPTHREAD_START_ROUTINE)&AccepteRoutine, this, 0, &m_AcceptThreadID);

	return (m_IsListening = true);
}
bool CWinSock_Server::Stop()
{
	if(!m_IsListening)
		return false;
	
	if(m_ListeningSocket != INVALID_SOCKET)
		if(closesocket(m_ListeningSocket))
		{
			printf("Close Listening Socket Failure\n");
			PrintWSAError
			return false;
		}

	/** WaitForThreadExit */
	WaitForSingleObject(m_hAcceptThread, INFINITE);
	CloseHandle(m_hAcceptThread);
	m_hAcceptThread = INVALID_HANDLE_VALUE;
	
	/** Clean Client List */
	SetEvent(m_CommandEventTab[ActionIDS_CleanClientList]);

	return (m_IsListening = false);
}
void CWinSock_Server::CleanClientList()
{
	/** Clean CLient List */
	while(m_pClientList->Count())
	{
		CWinSock_ServerClient * pClient = (CWinSock_ServerClient *)m_pClientList->GetFirst();
		if(pClient)
			delete pClient;

		m_pClientList->DeleteFirst();
	}
}
void CWinSock_Server::PurgeClientList()
{
	if(!m_pClientList)
		return;

	for(unsigned int i = 0 ; i < m_pClientList->Count() ; ++i)
	{
		CWinSock_ServerClient * pClient = (CWinSock_ServerClient *)(*m_pClientList)[i];

		if(!pClient)
		{
			m_pClientList->Delete(pClient);
			printf("Client pointer empty Delete it. %u %s Allready In List\n", m_pClientList->Count(), (m_pClientList->Count()  > 1  ? "Clients":"Client"));
			--i;
			continue;
		}

		if(!pClient->Connected())
		{
			m_pClientList->Delete(pClient);
			printf("Client %u Disconnected Delete it from list. %u %s Allready In List\n",pClient->Socket(), m_pClientList->Count(), (m_pClientList->Count()  > 1  ? "Clients":"Client"));
			delete pClient;
			i = (unsigned int)-1;//--i;
		}
	}
}
void CWinSock_Server::ClientDisconnected()
{
	SetEvent(m_CommandEventTab[ActionIDS_PurgeCLientList]);
}
/** Action thread Routine */
DWORD ActionThreadRoutine(LPVOID lpdwThreadParam )
{
	printf("Accept routine Thread Started\n");
	DWORD WaitResult = 0;
	if(lpdwThreadParam)
	{
		CWinSock_Server * pThis = (CWinSock_Server *)lpdwThreadParam;
		while(1)
		{
			WaitResult = WaitForMultipleObjects(MAX_ActionIDS, pThis->m_CommandEventTab, false, INFINITE);
			
			printf("%s\n", ActionIDS_STR_TAB[WaitResult]);

			printf("Simulate Prior Task\n");
			Sleep(10000);

			/** Treat Request */
			switch(WaitResult)
			{
				case ActionIDS_CleanClientList:
					pThis->CleanClientList();
					break;
				case ActionIDS_PurgeCLientList:
					pThis->PurgeClientList();
					break;
				default:
					printf("Unknown Event\n");
					break;

			}

			/** Reset Event */
			ResetEvent(pThis->m_CommandEventTab[WaitResult]);

			/** Exit if Requested */
			if(WaitResult == ActionIDS_EXIT)
			{
				printf("Exit Action Thread Request\n");
				break;
			}
		}
	}

	ExitThread(0);

	return 0;
}
/** Accept Thread Routine */
DWORD AccepteRoutine(LPVOID lpdwThreadParam )
{
	printf("Accept routine Thread Started\n");
	int SockOptParam = 0;
	if(lpdwThreadParam)
	{
		CWinSock_Server * pThis = (CWinSock_Server *)lpdwThreadParam;
		while(1)
		{
			struct sockaddr NewClientInfos = {0};
			SOCKET NewClientSocket = INVALID_SOCKET;
			int AddrLen = sizeof(NewClientInfos);

			NewClientSocket = accept(pThis->m_ListeningSocket, &NewClientInfos, &AddrLen);
			if(NewClientSocket == INVALID_SOCKET)
			{
				printf("Client Acceptation Failure\n");
				PrintWSAError
				break;
			}
			
#ifdef SET_SOCK_OPTION_ON_CONNEXION_ACCEPT
			/** Configure Accepted Socket */
			setsockopt(NewClientSocket, SOL_SOCKET, SO_LINGER, (char *)&SockOptParam, sizeof(SockOptParam));
#endif

			CWinSock_ServerClient * pNewClient = new CWinSock_ServerClient(NewClientSocket, pThis, true);

			pThis->m_pClientList->Add(pNewClient);

			pNewClient->Start();
			printf("New Client Accepted. %u %s in list\n", pThis->m_pClientList->Count() , (pThis->m_pClientList->Count() > 1 ? "Clients":"Client"));
		}
	}
	printf("Exiting Accept Routine thread\n");
	ExitThread(0);
	return 0;
}
