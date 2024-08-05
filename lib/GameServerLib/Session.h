#pragma once
#include "RingBuffer.h"
#include "type.h"
#include "LockFreeQueue.h"
#include <queue>


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

	~Session()
	{
		
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

	// ip
	WCHAR _ip[INET_ADDRSTRLEN] = L"";
	USHORT _port = 0;
	// �۽�������
	bool _isSending = false;
	// ���� ������ io count
	IoFlag _ioFlag;
	bool _disconnectRequested = false;
	
	//GameThread
	class GameThread* _gameThread;
	LockFreeQueue<CPacket*> _packetQueue;
	uint16 _gameThreadArrIndex;
	SRWLOCK _lock;

	void Init(int64 sessionId, SOCKET socket, SOCKADDR_IN* clientAddr);
	void Init(int64 sessionId, SOCKET socket);

	void ClearSendedQueue();
	void ClearSendQueue();
	void ClearPacketQueue();

	//long _bProcessingAsyncJobNum = 0;
};



