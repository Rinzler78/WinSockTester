#pragma once
#include "Winsock2.h"
#include "WinSock_Test_Option.h"


class CI_WinSock_ServerClient_Delegate
{
public:
	virtual void ClientDisconnected() = NULL;
};

class CWinSock_ServerClient
{
public:
	CWinSock_ServerClient(SOCKET NewClientSocket, CI_WinSock_ServerClient_Delegate * pDelegate, bool EchoMode);
	~CWinSock_ServerClient(void);

	bool Start();
	bool Stop();
	inline bool Connected() const {return m_Connected;}
	inline SOCKET Socket() const {return m_ClientSocket;}
private:
	bool BeginReceive();
	bool BeginSend(unsigned char * pBuffer, unsigned int BufferSize);

	bool m_Connected;

	SOCKET m_ClientSocket;
	
	/** Reception */
	unsigned char * m_pReceptionBuffer;
	OVERLAPPED m_Reception_OverlapedStructure;
	HANDLE m_ReadCompletedEvent;
	void OnFrameReceived(unsigned int & NbBytesReceived);

	/** Send */
	OVERLAPPED m_Send_OverlapedStructure;
	HANDLE m_SendCompletedEvent;
	void OnFrameSent(unsigned int & NbBytesSent);
	
#ifdef USING_EXS_FUNCTIONS
	friend void WINAPI ReadFileCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
	friend void WINAPI WriteFileCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
#else
	HANDLE m_AsyncThread;
	DWORD m_AsyncThreadID;
	HANDLE m_ExitAsyncThreadEvent;
	unsigned int m_NbBytesReceived;
	unsigned int m_NbBytesSent;
	friend DWORD AsyncThreadRoutine(LPVOID lpdwThreadParam);
#endif
	/** Client Delegate */
	CI_WinSock_ServerClient_Delegate * m_pDelegate;

	/** Server Options */
	bool m_EchoMode;
};

