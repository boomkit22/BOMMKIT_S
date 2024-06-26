#pragma once
#include "RingBuffer.h"
#include "type.h"
#include "LockFreeQueue.h"
#include <queue>

//#define SESSION_LOG_MODE 

#define MAX_SESSION_LOG 10000

struct SessionLog
{
	uint32 _code;
	uint32 _line;
};

//���漱��
class CPacket;

struct IoFlag {
	long _releaseFlag;
	long _ioCount;

};

struct Session
{
	Session()
	{
		_ioFlag._releaseFlag = 0;
		_ioFlag._ioCount = 1;
	}
	int64 _sessionId;
	// send�� overlapp
	OVERLAPPED _sendOverlapped;
	// recv�� overlapp
	OVERLAPPED _recvOverlapped;
	// ����
	SOCKET _socket = INVALID_SOCKET;
	// ���� ť
	RingBuffer _recvQueue;
	//�۽� ť
	LockFreeQueue<CPacket*> _sendQueue;
	std::queue<CPacket*> _sendedQueue;
	void ClearSendedQueue();
	// ip
	WCHAR _ip[INET_ADDRSTRLEN] = L"";
	USHORT _port = 0;
	// �۽�������
	bool _isSending = false;
	// ���� ������ io count
	IoFlag _ioFlag;
	/*long _ioCount = 0;*/
	bool _disconnectRequested = false;
	//uint32 _sendPacketNum = 0;
	void Init(int64 sessionId, SOCKET socket, SOCKADDR_IN* clientAddr);
	void Init(int64 sessionId, SOCKET socket);
};



// ������
// ���Ǻ� ����� �־�����


enum LogCode
{
	Accpet = 0,
	Disconnect = 1,
	RecvPost = 2,
	SendPost = 3,
	OnRecvPacket = 4,
	IncIo = 5,
	DecIo = 6
};

void LogSession(Session* session, SessionLog log);


#ifdef SESSION_LOG_MODE
#define LOG_SESSION(session, log) LogSession(session, log)
#else
#define LOG_SESSION(session, log)
#endif


