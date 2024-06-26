#include "CNetServer.h"
#include <stdio.h>
#include "PacketHeader.h"
#include "SerializeBuffer.h"
#include "LockGuard.h"
#include <process.h>
#include "Log.h"

CNetServer::CNetServer()
{
	//�����ڿ��� ���ұ�?
}

CNetServer::~CNetServer()
{
	//�Ҹ��ڿ��� ���ұ�?
}

bool CNetServer::Start(uint16 port, uint32 concurrentThreadNum, uint32 workerThreadNum, int nagle, int sendZeroCopy)
{
	LOG(L"System", LogLevel::System, L"Init Network Library Start");
	//wprintf(L"initnetwork start\n");
	_serverPort = port;
	_nagle = nagle;
	_sendZeroCopy = sendZeroCopy;

	// ���� �迭 �ʱ�ȭ
	InitSessionArr();
	int errorCode;

	// ���� iocp port
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrentThreadNum);
	if (_iocpHandle == NULL)
	{
		errorCode = WSAGetLastError();
		return false;
	}

	//worker thread
	HANDLE hWorkerThread;
	for (uint32 i = 0; i < workerThreadNum; i++)
	{
		hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadStatic, this, 0, NULL);
		if (hWorkerThread == NULL)
		{
			errorCode = WSAGetLastError();
			return false;
		}
		_threadList[_threadNumber++] = hWorkerThread;
	}

	//monitor thread
	HANDLE hMonitorThread;;
	hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThreadStatic, this, 0, NULL);
	if (hMonitorThread == NULL)
	{
		errorCode = WSAGetLastError();
		return false;
	}
	_threadList[_threadNumber++] = hMonitorThread;

	//accept thread
	HANDLE hAcceptThread;
	hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadStatic, this, 0, NULL);
	if (hAcceptThread == NULL)
	{
		errorCode = WSAGetLastError();
		return false;
	}
	_threadList[_threadNumber++] = hAcceptThread;


	LOG(L"System", LogLevel::System, L"Init Network Library End");
	return true;
}

void CNetServer::Stop()
{
	LOG(L"System", LogLevel::System, L"Stop Network Library Start");
	_bStopNetwork = true; // accept thread, monitor thread, release thread
	closesocket(_listenSocket); // accept thread
	PostQueuedCompletionStatus(_iocpHandle, 0, 0, nullptr); // worker thread
	WaitForMultipleObjects(_threadNumber, _threadList, true, INFINITE); // thread ��޸���
	ClearSesssionArr(); // session �迭 ����
	CloseHandle(_iocpHandle); // iocp handle �ݱ�
	LOG(L"System", LogLevel::System, L"Stop Network Library End");
}


void CNetServer::Disconnect(Session* session)
{
	session->_disconnectRequested = true;
	CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_recvOverlapped);
	CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_sendOverlapped);
}

void CNetServer::Disconnect(int64 sessionId)
{
	if (sessionId < 0)
		return;

	Session* session = GetSession(sessionId); 
	if (session == nullptr)
	{
		return;
	}

	session->_disconnectRequested = true;
	CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_recvOverlapped);
	CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_sendOverlapped);

	PutBackSession(session);
	return;
}



//Enqueue�� SendPost�� ���ؾ��ϳ�
void CNetServer::SendPacket(int64 sessionId, CPacket* packet)
{
	if (sessionId < 0)
		return;


	Session* session = GetSession(sessionId);
	if (session == nullptr)
	{
		return;
	}

	// �������κ��� disconnect ��û �����ſ�����
	if (session->_disconnectRequested)
	{
		// �ٽ� ����ְ� return
		// disconnect��û�� ���Ŀ� �������� ����  �ϴ� ���⼭ �ѹ� �Ÿ���
		PutBackSession(session);
		return;
	}

	InterlockedIncrement64(&_processSendPacket); // �α׿�

	// ��Ŷ�� ���� ���ǿ� ������ ���� ���Ǵ� packet refcount
	packet->IncRefCount();

	//���ڵ� �ϰ� ť�� �ְ�
	packet->Encode(_packetKey);
	session->_sendQueue.Enqueue(packet);

	SendPost(session);
	PutBackSession(session);
	return;
}

unsigned __stdcall CNetServer::WorkerThread()
{
	int retVal;

	while (true)
	{

		DWORD cbTransferred = 0;
		SOCKET clientSock = NULL;

		OVERLAPPED* overlapped = nullptr;
		Session* s = nullptr;

		retVal = GetQueuedCompletionStatus(_iocpHandle, &cbTransferred,
			(PULONG_PTR)&s, &overlapped, INFINITE);

	
		if (s == nullptr)
		{
			if (retVal == 0)
			{
				int errorCode = WSAGetLastError();
				LOG(L"WorkerThread", LogLevel::Error, L"GQCS ret 0");
			}

			// �ٸ� ��Ŀ���������� ����
			PostQueuedCompletionStatus(_iocpHandle, 0, 0, 0);
			break;
		}

		if (overlapped == (OVERLAPPED*)0x12345678)
		{
			ReqWSASend(s);
			continue;
		}

		if (cbTransferred == 0)
		{
			DecIoCount(s);
			continue;
		}


		if (overlapped == &(s->_sendOverlapped))
		{
			int32 networkSend = (int32)cbTransferred + (((int32)cbTransferred) / 1460 + 1) * 40;
			InterlockedAdd(&_networkSend, networkSend);
			{
				// session�� wsasend�� ť
				s->ClearSendedQueue();
				InterlockedExchange8((CHAR*)(&s->_isSending), false);
				SendPost(s);

			}
		}

		else if (overlapped == &(s->_recvOverlapped))
		{
			int32 networkRecv = (int32)cbTransferred + (((int32)cbTransferred) / 1460 + 1) * 40;
			InterlockedAdd(&_networkRecv, networkRecv);
			
			int moveRecvRearVal = s->_recvQueue.MoveRear(cbTransferred);
			if (moveRecvRearVal != cbTransferred)
			{
				// ringbuffer����
				__debugbreak();
			}


			while (true)
			{
				// ��Ŷ ��� ��ŭ �������� ���
				if (s->_recvQueue.GetUseSize() < sizeof(NetHeader))
				{
					break;
				}

				// �����ŭ �ϴ� Peek�ؿ��µ�
				NetHeader header;
				int peekVal = s->_recvQueue.Peek((char*)&header, sizeof(NetHeader));
				if (peekVal != sizeof(NetHeader))
				{
					__debugbreak();
				}

				// len��ŭ �������� ���
				int dataSize = header._len;
				if (s->_recvQueue.GetUseSize() < sizeof(NetHeader) + dataSize)
				{
					break;
				}

				// header peek�Ѱ� ���������� ó���ϰ�
				int moveHeaderVal = s->_recvQueue.MoveFront(sizeof(NetHeader));
				if (moveHeaderVal != sizeof(NetHeader))
				{
					__debugbreak();
				}

				// packet�� ����
				CPacket* packet = CPacket::Alloc();
				char* buf = packet->GetBufferPtr();
				// datasize��ŭ �ܾ����
				s->_recvQueue.Dequeue(buf, dataSize);
				packet->MoveWritePos(dataSize);
				//���ڵ�  ( checksum ���� )
				bool bDecodeSucceed = packet->Decode(&header, _packetKey);
				if (!bDecodeSucceed)
				{
					//checksum ����������
					//dec io count�� �ϰ� �ٽ� recvpost�� �������ν� ������ �����Ѵ�
					Disconnect(s);
				}
				// ��Ŷ �������� �ѱ��
				OnRecvPacket(s->_sessionId, packet);
				InterlockedIncrement64(&_processRecvPacket);
			}
			if (!s->_disconnectRequested)
			{
				//checksum ����������
				//dec io count�� �ϰ� �ٽ� recvpost�� �������ν� ������ �����Ѵ�
				RecvPost(s);
			}
		}
		else {
			__debugbreak();
		}

		// �Ϸ������� iocount�� 1 ����
		// �ٽ� recv Post, ��� �����̵� session�� ����� �������� recv post�� iocount�� 1�̻� �����Ѵ�
		DecIoCount(s);
	}

	return 0;
}


unsigned __stdcall CNetServer::AcceptThread()
{
	LOG(L"System", LogLevel::System, L"Accept Thread Start");
	// ���� ip port
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(_serverPort);

	// ���� ���� �����
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		return 0;
	}

	// linger fin ������
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	int optRet = setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		__debugbreak();
		return false;
	}

	// zero copy �ɼ� �߰�������
	if (_sendZeroCopy == 1)
	{
		int sendZeroCopy = 0; // �۽� ���� ũ�⸦ 0���� ����
		int retVal = setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendZeroCopy, sizeof(sendZeroCopy));
		if (retVal == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			wprintf(L"setsockopt sndbuf failed : %d\n", error);
			__debugbreak();
		}
	}
	

	// bind�ϰ�
	int bindRetVal;
	bindRetVal = bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRetVal == SOCKET_ERROR)
	{
		//bind failed
		wprintf(L"bind failed\n");
		return false;
	}

	int listenRetVal;
	listenRetVal = listen(_listenSocket, SOMAXCONN_HINT(10000));
	if (listenRetVal == SOCKET_ERROR)
	{
		// listen failed
		wprintf(L"listend failed\n");
		return false;
	}

	
	while (!_bStopNetwork)
	{
		int addrLen;
		 //clientSocket;

		SOCKADDR_IN clientAddr;

		//accept
		addrLen = sizeof(clientAddr);
		SOCKET clientSocket = accept(_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			/*
			*  listen socket ������ ����¿��� 10004

				WSAEINTR
				10004
				WSACancelBlockingCall
			*/
			if (_bStopNetwork)
			{
				LOG(L"System", LogLevel::System, L"Accept Thread End By Keyboard Input");
			}
			else {
				LOG(L"System", LogLevel::System, L"Accept Error");
			}
			break;
		}

		_totalAccept++;
		_acceptPacket++;

		bool bAcceptSuccess = AcceptProcess(clientSocket, &clientAddr);
		if (!bAcceptSuccess)
		{
			/*__debugbreak();*/
		}
	}
	LOG(L"System", LogLevel::System, L"Accept Thread End");
	return 0;
}



unsigned __stdcall CNetServer::MonitorThread()
{
	int tpsIndex = 0;
	while (!_bStopNetwork)
	{
		Sleep(1000);
		_monitoredSendTPS[tpsIndex % TPS_ARR_NUM] = _processSendPacket;
		_processSendPacket = 0;

		_monitoredRecvTPS[tpsIndex % TPS_ARR_NUM] = _processRecvPacket;
		_processRecvPacket = 0;

		_monitoredAcceptTPS[tpsIndex % TPS_ARR_NUM] = _acceptPacket;
		_acceptPacket = 0;

		_monitoredNetworkByteSendTPS[tpsIndex % TPS_ARR_NUM] = _networkSend;
		_networkSend = 0;

		_monitoredNetworkByteRecvTPS[tpsIndex % TPS_ARR_NUM] = _networkRecv;
		_networkRecv = 0;

		tpsIndex++;
	}
	return 0;
}


bool CNetServer::SendPost(Session* session)
{
	IncIoCount(session);
	int useSize = session->_sendQueue.Size();
	if (useSize == 0)
	{
		DecIoCount(session);
		return false;
	}

	if (_InterlockedExchange8((char*)(&session->_isSending), true) == true)
	{
		DecIoCount(session);
		return false;
	}

	if (session->_sendQueue.Size() == 0)
	{
		InterlockedExchange8((char*)(&session->_isSending), false);
		DecIoCount(session);
		return false;
	}

	PostQueuedCompletionStatus(_iocpHandle, 0, ULONG_PTR(session), (LPOVERLAPPED)0x12345678);
	return true;
}


//���� ��û
bool CNetServer::RecvPost(Session* session)
{
	IncIoCount(session);
	SessionLog sessionLog = { LogCode::RecvPost, (uint32)__LINE__ };

	LOG_SESSION(session, sessionLog);
	//WriteLock writeLock(&session->_sessionLock);
	int directEnqueueSize = session->_recvQueue.GetDirectEnqueueSize();
	// ���Ź��۰� �����°� ����, ������ �ٷ� ó���ؾ��ϴϱ�
	// �ٵ� ���� ������ �񵿱� i/o������ ���� �� �� �ִ°ǰ�

	if (directEnqueueSize == 0)
	{
		__debugbreak();
	}
	char* rearPtr = session->_recvQueue.GetRearPtr();


	int wsabufNum = 1;

	// bufferptr
	// [] [] [front ptr] [] [] [] [] [rearptr] [] [] [] [] []
	// rearptr���� �ڿ�����
	// �տ����� frontptr����

	WSABUF wsabufArr[2];
	//WSABUF wsabufRear;
	wsabufArr[0].buf = rearPtr;
	wsabufArr[0].len = directEnqueueSize;

	int canReceiveSize = session->_recvQueue.GetFreeSize();
	if (canReceiveSize > directEnqueueSize)
	{
		wsabufNum = 2;
		char* bufferPtr = session->_recvQueue.GetBufferPtr();

		wsabufArr[1].buf = bufferPtr;
		wsabufArr[1].len = canReceiveSize - directEnqueueSize;
	}

	//printf("recv post receiveSize : %d\n", canReceiveSize);
	//DWORD recvBytes;
	DWORD flags;
	flags = 0;

	// recv overlapp
	memset(&(session->_recvOverlapped), 0, sizeof(session->_recvOverlapped));

	//�񵿱�� ��û ������
	if (session->_disconnectRequested)
	{
		DecIoCount(session);
		return false;
	}

	int retVal = WSARecv(session->_socket, wsabufArr, wsabufNum, nullptr, &flags, &session->_recvOverlapped, NULL);
	if (retVal == SOCKET_ERROR)
	{
		int recvError = WSAGetLastError();
		if (recvError != ERROR_IO_PENDING)
		{
			//CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_sendOverlapped);
			//TODO: �α����
			if (recvError != WSAECONNRESET && recvError != WSAECONNABORTED)
			{
				LOG(L"Disconnect", LogLevel::Error, L"WSA Recv error : %d, errorCode :%d", session->_sessionId, recvError);
			}
		/*	if (recvError == WSAECONNRESET)
			{
				InterlockedIncrement64(&_recvConnResetTotal);
				LOG(L"Disconnect", LogLevel::Debug, L"Recv WSAECONNRESET : %d", session->_sessionId);
			}*/

			DecIoCount(session);
			return false;
		}
		else {
			//�񵿱�� ��û ������
			if (session->_disconnectRequested)
			{
				CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_recvOverlapped);
				//DecIoCount(session);
				return false;
			}
		}
		// ERROR IO PENDING�̸� ����
	}
	return true;
}

bool CNetServer::AcceptProcess(SOCKET socket, SOCKADDR_IN* clientAddr)
{
	// ���� �����ϰ�
	Session* session = CreateSession(socket, clientAddr);
	if (session == nullptr)
	{
		return false;
	}
	/*if (session->_ioFlag._ioCount != 1)
	{
		__debugbreak();
	}*/

	// iocp�� ����ϰ�
	HANDLE iocpHandle = CreateIoCompletionPort((HANDLE)socket, _iocpHandle, (ULONG_PTR)session, 0);
	if (iocpHandle == NULL)
	{
		//error code 6 �̹� ������� ���� ����Ϸ��� ��
		int errorCode = WSAGetLastError();

		LOG(L"Accept", LogLevel::Error, L"connect process create io completion port failed : %d\n", errorCode);
		return false;
	}

	// ������ accept ó��
	OnAccept(session->_sessionId);

	SessionLog sessionLog = { LogCode::Accpet, (uint32)__LINE__ };
	LOG_SESSION(session, sessionLog);
	
	//recv post �ɰ�
	RecvPost(session);

	// iocount 1 ���ҽ�Ŵ , session�� ���۵����� iocount 1�̱� ������ recvpost�� ���� 2�̰�, ���⼭ ���ҽ��Ѿ� 1��
	bool bDisconnected = DecIoCount(session);
	if (bDisconnected)
	{
		return false;
	}

	return true;
}


Session* CNetServer::CreateSession(SOCKET socket, SOCKADDR_IN* clientAddr)
{
	// 1�� �����ϴ� sessionid
	static int64 GenerateSessionId = 0;
	int64 id = ++GenerateSessionId;
	uint16 sessionIndex = 0;

	//�� index ã�Ƽ�
	bool bPopSucceed = _emptySessionIndex.Pop(sessionIndex);
	if (!bPopSucceed)
	{
		// TODO: ������ ����ȹްų�
		// �ø��ų�
		// ������ Disconnect�� ��û�ؼ� ��������
		// ���� ������ �� �����Ҽ�������

		//__debugbreak();
		LOG(L"Accept", LogLevel::Error, L"!bPopSucceed from empty session index");
		return nullptr;
	}
	else {
		InterlockedIncrement64(&_sessionNum);
	}
	 
	//���� id 8 byte�� �տ� 2byte sessionInde�� ����
	uint16* ptr = (uint16*)&id;
	ptr += 3; 
	*ptr = sessionIndex;


	Session* session = nullptr;
	{
		session = &_sessionArr[sessionIndex];
	}

	session->Init(id, socket, clientAddr);
	return session;
}


// close socket�� 1ȸ�ϱ� ���ؼ�
bool CNetServer::ReleaseSession(Session* session)
{
	uint64 sessionId = session->_sessionId;
	uint16 sessionIndex = GetSessionIndex(sessionId);

	int closeSocketError;
	int closeSocketErrorCode;
	{
		//WriteLock sessionLock(&session->_sessionLock);
		closeSocketError = closesocket(session->_socket);
		if (closeSocketError != 0)
		{
			closeSocketErrorCode = WSAGetLastError();
			__debugbreak();
		}
	}

	session->ClearSendQueue();
	session->ClearSendedQueue();


	_emptySessionIndex.Push(sessionIndex);
	InterlockedDecrement64(&_sessionNum);
	InterlockedIncrement64(&_totalDisconnet);
	return true;
}

/*
Release�ÿ� ioFlag 1�λ��·�
// 1������Ű�� �ִ� ����:
	// session�� iocount�� �����ҋ� 1�� �����Ѵ�
	// ���� io count�� 0�̶�� session ��Ȱ�� ��������
	// ���� id ���� ����
	//  contents���� ���� �ش� ���� id�� ����ְ� sendpacket�� �õ��ϸ�
	// ���� �������� sendpacket���� iocount ������Ű�� ���ҽ�Ű�� 0�̵Ǵµ�
	// 0�̵Ǹ� �ٷ� ���� ��������� ������
	// �⺻�� 1�ν����ϰ� accept���� recvpost�Ϸ��� 1���ҽ�Ų�� (recvpost���� 2�� �������⶧����)
*/
bool CNetServer::TryReleaseSession(Session* session, int line)
{
	// ��ǥ, �����Ұ�, ����
	// ���� ioFlag = 0, ioCount = 0;
	// �����Ұ� ioFlag = 1, ioCount = 1;
	int64 exchangeValue = 0x0000'0001'0000'0001;
	if (InterlockedCompareExchange64((int64*)(&session->_ioFlag), exchangeValue, 0) != 0)
	{
		//ioFlag�� 0�� �ƴϾ�����
		// ���� ������� ������: relase ������
		return false;
	}
	LOG(L"Disconnect", LogLevel::Debug, L"ReleaseSession session Id: sessiond Id :%d, line :  %d", session->_sessionId, line);
	OnDisconnect(session->_sessionId);
	ReleaseSession(session);
	return true;
}

//inline ó�� �ư�
inline uint16 CNetServer::GetSessionIndex(int64 sessionId)
{
	//uint16* ptr = (uint16*)&sessionId;
	/*ptr += 3;
	return *ptr;*/
	return *((uint16*)&sessionId + 3);
}


inline void CNetServer::IncIoCount(Session* session)
{
	InterlockedIncrement(&session->_ioFlag._ioCount);
	/*SessionLog sessionLog = { LogCode::IncIo, after };
	LOG_SESSION(session, sessionLog);*/
}


/// <summary>
/// 
/// </summary>
/// <param name="session"></param>
/// <returns> return true when disconnected</returns>
inline bool CNetServer::DecIoCount(Session* session)
{
	if (InterlockedDecrement(&session->_ioFlag._ioCount) == 0)
	{
		TryReleaseSession(session, (uint32)__LINE__);
		return false;
	}
	
	return true;
}

inline Session* CNetServer::GetSession(int64 sessionId)
{
	//Release�� lock���� ����ȭ �ϱ� ����
	// ioCount�� ������Ų���� release�� ���ϰ� �� ������ ���Լ��� ������

	// io count�� 1��
	// io count�� 1�̸� release�� ������
	// ���� �׳� ioCount ����������
	// �ٵ� relaseFlag �̹� true��
	// �׷� �̼����� release�� �����̰ų� release ���� �����̰�
	// ���� �������ߴ� �����̵�. �ٸ� �����̵�
	// �׳� decremnet io count�ϰ� �������

	// release���� �����̾��� -> iocount ���ҽ�Ű�� �������
	// release�� �����̾��� -> iocount ���ҽ�Ű�� �������
	// release�ǰ� ���� �Ǿ��� -> iocount ���ҽ�Ű�� �������
		//uint16 sessionIndex = 
		/*if (sessionIndex < 0 || sessionIndex > MAX_SESSION_NUM)
			return nullptr;*/
	Session* s = &_sessionArr[GetSessionIndex(sessionId)];

	IncIoCount(s);
	//�̹� ������¸�
	if (s->_ioFlag._releaseFlag == 1 || sessionId != s->_sessionId)
	{
		DecIoCount(s);
		return nullptr;
	}
	return s;
}

inline void CNetServer::PutBackSession(Session* session)
{
	DecIoCount(session);
}

void CNetServer::ReqWSASend(Session* session)
{
	int retVal;
	int checkedQueueSize = session->_sendQueue.Size();
	// �ٶ� size��ŭ dequeue�� ���Ҽ� �ִµ�

	/*if (useSize % 8 != 0)
	{
		__debugbreak();
	}*/

	memset(&(session->_sendOverlapped), 0, sizeof(session->_sendOverlapped));

	if (checkedQueueSize > MAX_PACKET_NUM_IN_SEND_QUEUE)
	{
		LOG(L"Disconnect", LogLevel::Error, L"sendqueue full session Id : %lld", session->_sessionId);
		InterlockedIncrement64(&_disconnectSessionNumBySendQueueFull);
		DecIoCount(session);
		Disconnect(session);
		return;
	}

	WSABUF wsaSendBuf[MAX_PACKET_NUM_IN_SEND_QUEUE];
	int dequeuedSize = 0;
	for (int i = 0; i < checkedQueueSize; i++)
	{

		CPacket* ptr = nullptr;
		bool bDequeueSucceed = session->_sendQueue.Dequeue(ptr);
		if (!bDequeueSucceed)
		{
			//����������
			break;
		}
		wsaSendBuf[dequeuedSize].buf = ptr->GetBufferPtr();
		wsaSendBuf[dequeuedSize].len = ptr->GetDataSize();
		dequeuedSize++;

		if (ptr->_refCount == 0)
		{
			__debugbreak();
		}
		session->_sendedQueue.push(ptr);
	}

	int wsabufNum = dequeuedSize;

	if (session->_disconnectRequested)
	{
		DecIoCount(session);
		return;
	}


	retVal = WSASend(session->_socket, wsaSendBuf, wsabufNum, nullptr, 0, &session->_sendOverlapped, NULL);

	if (retVal == SOCKET_ERROR)
	{
		int sendError = WSAGetLastError();
		if (sendError != WSA_IO_PENDING)
		{
			//CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_recvOverlapped);

			// TODO: �α� ���
			if (sendError != WSAECONNRESET && sendError != WSAECONNABORTED)
			{
				LOG(L"Disconnect", LogLevel::Error, L"WSA SendError: %d, errorCode :%d", session->_sessionId, sendError);
			}

			//if (sendError == WSAECONNRESET)
			//{
			//	LOG(L"Disconnect", LogLevel::Debug, L"Send WSAECONNRESET : %d", session->_sessionId);
			//}
			DecIoCount(session);
			return;

		}
		else
		{
			//�񵿱�� ��û ������
			if (session->_disconnectRequested)
			{
				CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_sendOverlapped);
				//DecIoCount(session);
				return;

			}
			//DecIoCount(session);  �Ϸ������� ���Ŵϱ� �Ϸ��������� DecIoCount�ؼ� ���⼭ �ϸ�ȵ�
			return;
		}
	}
}


int64 CNetServer::GetSessionId(int64 sessionId)
{
	//6 byte
	// 0x0000'FFFF'FFFF'FFFF
	int64 demaskValue = 0x0000'FFFF'FFFF'FFFF;
	return sessionId & demaskValue;
}


void CNetServer::InitSessionArr()
{
	//memset(_sessionArr, 0, sizeof(_sessionArr));
	//InitializeCriticalSection(&_sessionIndexLock);
	for (int i = MAX_SESSION_NUM - 1; i >= 0; i--)
	{
		_emptySessionIndex.Push(i);
	}
}

void CNetServer::ClearSesssionArr()
{
	//TODO:  ���� �Ѵٸ�
	// packet queue�� ������ �ݳ�
	// close socket
}