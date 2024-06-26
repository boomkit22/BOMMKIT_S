#include "CLanClient.h"
#include <stdio.h>
#include "SerializeBuffer.h"
#include "LockGuard.h"
#include <process.h>
#include "Log.h"

CLanClient::CLanClient()
{
	//�����ڿ��� ���ұ�?
}

CLanClient::~CLanClient()
{
	//�Ҹ��ڿ��� ���ұ�?
}



bool CLanClient::Start(const WCHAR* serverIp, uint16 port, uint32 concurrentThreadNum, uint32 workerThreadNum)
{
	LOG(L"System", LogLevel::System, L"Init CLanClient Start");
	memset(&_serverAddr, 0, sizeof(_serverAddr));
	_serverAddr.sin_family = AF_INET;
	InetPtonW(AF_INET, serverIp, &_serverAddr.sin_addr);
	_serverAddr.sin_port = htons(port);

	_serverPort = port;
	hReleaseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	int errorCode;

	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrentThreadNum);
	if (_iocpHandle == NULL)
	{
		errorCode = WSAGetLastError();
		return false;
	}


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



	HANDLE hMonitorThread;;
	hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThreadStatic, this, 0, NULL);
	if (hMonitorThread == NULL)
	{
		errorCode = WSAGetLastError();
		return false;
	}
	_threadList[_threadNumber++] = hMonitorThread;

	LOG(L"System", LogLevel::System, L"Init CLanClient End");
	return true;
}

void CLanClient::Stop()
{
	LOG(L"System", LogLevel::System, L"Stop CLanClient Start");
	closesocket(_socket);
	_bStopNetwork = true;
	PostQueuedCompletionStatus(_iocpHandle, 0, 0, nullptr);
	WaitForMultipleObjects(_threadNumber, _threadList, true, INFINITE);
	CloseHandle(_iocpHandle);
	LOG(L"System", LogLevel::System, L"Stop CLanClient End");
}


void CLanClient::Disconnect()
{
	// TODO: session ã�Ƽ�
	// interlocked increment�ؼ� �� �������� �ϰ�
	// send overlapp ����
	// recv overlapp ����
	// ���̻� send�� recv ���ϰ��ϰ�
	IncIoCount(_clientSession);
	//�̹� ������¸�

	if (_clientSession->_ioFlag._releaseFlag == 1)
	{
		DecIoCount(_clientSession);
		return;
	}

	_clientSession->_disconnectRequested = true;
	CancelIoEx((HANDLE)_clientSession->_socket, (LPOVERLAPPED)&_clientSession->_recvOverlapped);
	CancelIoEx((HANDLE)_clientSession->_socket, (LPOVERLAPPED)&_clientSession->_sendOverlapped);

	DecIoCount(_clientSession);
}


//Enqueue�� SendPost�� ���ؾ��ϳ�

bool CLanClient::SendPacket(CPacket* packet)
{

	InterlockedIncrement64(&_processSendPacket); // �α׿�
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

	IncIoCount(_clientSession);
	//�̹� ����Ǽ� �ٸ��ָ�
	//�̹� ������¸�
	if (_clientSession->_ioFlag._releaseFlag == 1)
	{
		DecIoCount(_clientSession);
		return false;
	}



	if (_clientSession->_disconnectRequested)
	{
		DecIoCount(_clientSession);
		return false;
	}

	packet->IncRefCount();
	packet->Encode(30);
	_clientSession->_sendQueue.Enqueue(packet);



	SendPost(_clientSession);
	DecIoCount(_clientSession);
	return true;
}


unsigned __stdcall CLanClient::WorkerThread()
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

		if (overlapped == &(s->_sendOverlapped))
		{

			{
				// session�� wsasend�� ť
				s->ClearSendedQueue();
				InterlockedExchange8((CHAR*)(&s->_isSending), false);
				SendPost(s);

			}
		}

		else if (overlapped == &(s->_recvOverlapped))
		{


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
				bool bDecodeSucceed = packet->Decode(&header, _packeyKey);
				if (!bDecodeSucceed)
				{
					//checksum ����������
					//dec io count�� �ϰ� �ٽ� recvpost�� �������ν� ������ �����Ѵ�
					Disconnect();
				}
				// ��Ŷ �������� �ѱ��
				OnRecvPacket(packet);
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



unsigned __stdcall CLanClient::MonitorThread()
{
	LOG(L"Client", LogLevel::System, L"Monitor Thread Start");

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

		tpsIndex++;
	}
	return 0;
}



bool CLanClient::SendPost(Session* session)
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
bool CLanClient::RecvPost(Session* session)
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
			//TODO: �α����
			if (recvError != WSAECONNRESET && recvError != WSAECONNABORTED)
			{
				LOG(L"Client", LogLevel::Debug, L"WSA Recv error errorCode :%d", recvError);
				return false;
			}

			DecIoCount(session);
			return false;
		}
		else {
			//�񵿱�� ��û ������
			if (session->_disconnectRequested)
			{
				CancelIoEx((HANDLE)session->_socket, (LPOVERLAPPED)&session->_recvOverlapped);
				return false;
			}
		}
		// ERROR IO PENDING�̸� ����
	}
	return true;
}

void CLanClient::ReqWSASend(Session* session)
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
		DecIoCount(session);
		Disconnect();
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


bool CLanClient::Connect()
{
	int retVal;

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
	{
		wprintf(L"socket create failed\n");
		return false;
	}

	Session* session = CreateSession(_socket);
	if (session == nullptr)
	{
		return false;
	}

	retVal = connect(_socket, (SOCKADDR*)&_serverAddr, sizeof(_serverAddr));
	if (retVal == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		wprintf(L"connect failed : %d\n", errorCode);
		closesocket(_socket);
		return false;
	}

	HANDLE iocpHandle = CreateIoCompletionPort((HANDLE)_socket, _iocpHandle, (ULONG_PTR)_clientSession, 0);
	if (iocpHandle == NULL)
	{
		//error code 6 �̹� ������� ���� ����Ϸ��°Ͱ��v����
		int errorCode = WSAGetLastError();
		wprintf(L"connect process create io completion port failed : %d\n", errorCode);
		__debugbreak();
		closesocket(_socket);
		return false;
	}

	OnConnect();
	bool recvPostSucceed = RecvPost(_clientSession);
	if (!recvPostSucceed)
	{
		closesocket(_socket);
		return false;
	}

	DecIoCount(session);


	return true;
}

Session* CLanClient::CreateSession(SOCKET socket)
{
	static int64 GenerateSessionId = 0;
	int64 id = ++GenerateSessionId;


	_clientSession = new Session;
	_clientSession->Init(id, socket);
	return _clientSession;
}

bool CLanClient::ReleaseSession(Session* session)
{
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

	for (;;)
	{
		CPacket* packet;
		bool bDequeueSuceed = session->_sendQueue.Dequeue(packet);
		if (!bDequeueSuceed)
			break;
		CPacket::Free(packet);
	}
	if (session->_sendQueue.Size() != 0)
	{
		__debugbreak();
	}
	session->ClearSendedQueue();

	//EnterCriticalSection(&_sessionIndexLock);
	InterlockedDecrement64(&_sessionNum);
	//LeaveCriticalSection(&_sessionIndexLock);
	InterlockedIncrement64(&_totalDisconnet);
	delete session;
	return true;
}

bool CLanClient::TryReleaseSession(Session* session, int line)
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
	OnDisconnect();
	ReleaseSession(session);
	return true;
}


inline void CLanClient::IncIoCount(Session* session)
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
inline bool CLanClient::DecIoCount(Session* session)
{
	if (InterlockedDecrement(&session->_ioFlag._ioCount) == 0)
	{
		TryReleaseSession(session, (uint32)__LINE__);
		return false;
	}

	return true;
}
