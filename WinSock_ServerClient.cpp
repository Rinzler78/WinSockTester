#include "WinSock_ServerClient.h"
#include "stdio.h"

const char TEST_BUFFER [] = "Buffer Sent from WinSockTester";
const unsigned int TEST_BUFFER_SIZE = sizeof(TEST_BUFFER);

CWinSock_ServerClient::CWinSock_ServerClient(SOCKET NewClientSocket, CI_WinSock_ServerClient_Delegate * pDelegate, bool EchoMode)
:m_ClientSocket(NewClientSocket)
, m_pReceptionBuffer(new unsigned char [ReceptionBufferSize])
, m_ReadCompletedEvent(CreateEvent(NULL, false, false, NULL))
, m_SendCompletedEvent(CreateEvent(NULL, false, false, NULL))
#ifndef USING_EXS_FUNCTIONS
, m_AsyncThread(NULL)
, m_AsyncThreadID(0)
, m_NbBytesReceived(0)
, m_NbBytesSent(0)
, m_ExitAsyncThreadEvent(CreateEvent(NULL, false, false, NULL))
#endif
, m_Connected(true)
, m_pDelegate(pDelegate)
, m_EchoMode(EchoMode)
{
	memset(&m_Reception_OverlapedStructure, 0, sizeof(m_Reception_OverlapedStructure));
	memset(&m_Send_OverlapedStructure, 0, sizeof(m_Send_OverlapedStructure));

#ifndef USING_EXS_FUNCTIONS
	m_Reception_OverlapedStructure.hEvent = m_ReadCompletedEvent;
	m_Send_OverlapedStructure.hEvent = m_SendCompletedEvent;
#endif
}
CWinSock_ServerClient::~CWinSock_ServerClient(void)
{
	/** Stop Client Exchange */
	Stop();

	/** Free Memory from reception Buffer */
	delete [] m_pReceptionBuffer;

	/** Free Event Handles */
	if(m_ReadCompletedEvent)
		CloseHandle(m_ReadCompletedEvent);

	if(m_SendCompletedEvent)
		CloseHandle(m_SendCompletedEvent);
#ifndef USING_EXS_FUNCTIONS
	if(m_ExitAsyncThreadEvent)
		CloseHandle(m_ExitAsyncThreadEvent);
#endif

	/** Close The Client Socket */
	if(m_ClientSocket != INVALID_SOCKET)
		if(closesocket(m_ClientSocket))
			printf("CLose Socket Client Failure\n");
}

bool CWinSock_ServerClient::Start()
{
	if(m_ClientSocket == INVALID_SOCKET)
		return false;

	/** Start Reception Process */
	if(!BeginReceive())
	{
		printf("Begin receive Failure\n");
		return false;
	}

	/** Send Some bytes */
	if(!BeginSend((unsigned char *)TEST_BUFFER, TEST_BUFFER_SIZE))
	{
		printf("Begin Send Failure\n");
		return false;
	}

#ifndef USING_EXS_FUNCTIONS
	if(m_AsyncThread == NULL)
		m_AsyncThread = CreateThread(NULL, 128, (LPTHREAD_START_ROUTINE)&AsyncThreadRoutine, this, 0, &m_AsyncThreadID);

	if((m_AsyncThread == NULL) || (m_AsyncThread == INVALID_HANDLE_VALUE))
		return false;
#endif

	return true;
}
bool CWinSock_ServerClient::Stop()
{
	if(m_ClientSocket == INVALID_SOCKET)
		return false;

#ifndef USING_EXS_FUNCTIONS
	/** Stop Async Thread Request */
	SetEvent(m_ExitAsyncThreadEvent);
	
	/** Wait for Async thread exit */
	WaitForSingleObject(m_AsyncThread, INFINITE);

	/** Close Async thread Handle */
	CloseHandle(m_AsyncThread);
#endif
	if(!CancelIo((HANDLE)m_ClientSocket))
		printf("Cancel io For Socket %u failure\n", m_ClientSocket);

	/** Wait For Read Completed Event */
	if(m_ReadCompletedEvent != NULL)
		WaitForSingleObject(m_ReadCompletedEvent, INFINITE);

	/** Wait For Write Completed Event */
	if(m_SendCompletedEvent != NULL)
		WaitForSingleObject(m_SendCompletedEvent, INFINITE);

	return true;
}
bool CWinSock_ServerClient::BeginReceive()
{
	if(m_ClientSocket == INVALID_SOCKET)
		return false;

	printf("Begin Receive on socket %u\n", m_ClientSocket);
	
	memset(m_pReceptionBuffer, 0, ReceptionBufferSize);

	if(m_ReadCompletedEvent)
		ResetEvent(m_ReadCompletedEvent);

#ifdef USING_EXS_FUNCTIONS
	m_Reception_OverlapedStructure.hEvent = this; //Event not modified when using ex functions
	return ReadFileEx((HANDLE)m_ClientSocket, m_pReceptionBuffer, ReceptionBufferSize, &m_Reception_OverlapedStructure, (LPOVERLAPPED_COMPLETION_ROUTINE)ReadFileCompleted);
#else
	return (ReadFile((HANDLE)m_ClientSocket, m_pReceptionBuffer, ReceptionBufferSize, (LPDWORD)&m_NbBytesReceived, &m_Reception_OverlapedStructure) ?
			true:(GetLastError() == ERROR_IO_PENDING));
#endif
}
bool CWinSock_ServerClient::BeginSend(unsigned char * pBuffer, unsigned int BufferSize)
{
	if(!pBuffer || (BufferSize == 0))
		return false;

	printf("Begin Send on socket %u\n", m_ClientSocket);

	if(m_SendCompletedEvent)
		ResetEvent(m_SendCompletedEvent);

#ifdef USING_EXS_FUNCTIONS
	m_Send_OverlapedStructure.hEvent = this; //Event not modified when using ex functions
	return WriteFileEx((HANDLE)m_ClientSocket, pBuffer, BufferSize, &m_Send_OverlapedStructure, (LPOVERLAPPED_COMPLETION_ROUTINE)WriteFileCompleted);
#else
	return (WriteFile((HANDLE)m_ClientSocket, pBuffer, BufferSize, (LPDWORD)&m_NbBytesSent, &m_Send_OverlapedStructure) ? 
			true:(GetLastError() == ERROR_IO_PENDING));
#endif
}
void CWinSock_ServerClient::OnFrameReceived(unsigned int & NbBytesReceived)
{
	if(NbBytesReceived == 0)
	{
		printf("OnFrameReceived : Error code %u -> Client Disconnected\n", ERROR_NETNAME_DELETED);
		m_Connected = false;
		if(m_pDelegate)
			m_pDelegate->ClientDisconnected();
	}
	else
	{
		printf("%u %s received to Socket %u\n", NbBytesReceived, (NbBytesReceived > 1 ? "Bytes":"Byte"), m_ClientSocket);

		if(m_EchoMode)
			BeginSend(m_pReceptionBuffer, NbBytesReceived);
		
		BeginReceive();
	}

#ifdef USING_EXS_FUNCTIONS
	SetEvent(m_ReadCompletedEvent);
#endif
}
void CWinSock_ServerClient::OnFrameSent(unsigned int & NbBytesSent)
{
	if(NbBytesSent == 0)
	{
		printf("OnFrameSent : Error code %u -> Client Disconnected\n", ERROR_NETNAME_DELETED);
		m_Connected = false;
		if(m_pDelegate)
			m_pDelegate->ClientDisconnected();
	}
	else
	{
		printf("%u %s sent to Socket %u\n", NbBytesSent, (NbBytesSent > 1 ? "Bytes":"Byte"), m_ClientSocket);
	}

#ifdef USING_EXS_FUNCTIONS
	SetEvent(m_SendCompletedEvent);
#endif
}
#ifdef USING_EXS_FUNCTIONS
void WINAPI ReadFileCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	CWinSock_ServerClient * pThis = (CWinSock_ServerClient *)lpOverlapped->hEvent;
	printf("Read file ex Completed on socket %u\n", pThis->m_ClientSocket);
	pThis->OnFrameReceived((unsigned int &)dwNumberOfBytesTransfered);
}
void WINAPI WriteFileCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	CWinSock_ServerClient * pThis = (CWinSock_ServerClient *)lpOverlapped->hEvent;
	printf("Write file ex Completed on socket %u (%u %s sent)\n", pThis->m_ClientSocket, dwNumberOfBytesTransfered, (dwNumberOfBytesTransfered > 1 ? "Bytes":"Byte"));
	pThis->OnFrameSent((unsigned int &)dwNumberOfBytesTransfered);
}
#else
DWORD AsyncThreadRoutine(LPVOID lpdwThreadParam)
{
	if(lpdwThreadParam)
	{
		DWORD WaitReturn = 0;
		CWinSock_ServerClient * pThis = (CWinSock_ServerClient *)lpdwThreadParam;
		HANDLE EventTab[3] = {pThis->m_ExitAsyncThreadEvent, pThis->m_ReadCompletedEvent, pThis->m_SendCompletedEvent};

		while(1)
		{
			WaitReturn = WaitForMultipleObjects(3, EventTab, false, INFINITE);

			if((WaitReturn == 0) || (WaitReturn == WAIT_ABANDONED_0))
			{
				printf("Exit AsyncThreadRoutine\n");
				break;
			}

			printf("Reset Event %u -> %u\n", WaitReturn, EventTab[WaitReturn]);
			ResetEvent(EventTab[WaitReturn]);

			switch(WaitReturn)
			{
				case 1://Frame Received Flag
					GetOverlappedResult((HANDLE)pThis->m_ClientSocket, &pThis->m_Reception_OverlapedStructure, (LPDWORD)&pThis->m_NbBytesReceived, true);
					pThis->OnFrameReceived(pThis->m_NbBytesReceived);
					break;
				case 2://Frame Sent Flag
					pThis->OnFrameSent(pThis->m_NbBytesSent);
					break;
				default:
					break;
			}
		}

#ifndef USING_EXS_FUNCTIONS
		SetEvent(pThis->m_ReadCompletedEvent);
		SetEvent(pThis->m_SendCompletedEvent);
#endif
	}
	
	ExitThread(0);

	return 0;
}
#endif