#pragma once
#include "Type.h"
#include "Session.h"
#include <stack>
#include "LockFreeStack.h"
#include "Data.h"

// ��Ŭ�� : �׳� ���ڵ����ϰ� ���ڵ� ���ϰ�, len�� ���������� -> echoServeró��, üũ�� �˻���ϰ�
#define MAX_PACKET_NUM_IN_SEND_QUEUE 2000

class CPacket;

class CLanClient
{
public:
	CLanClient();
	virtual ~CLanClient();

	bool Connect();
	void Disconnect();
	bool SendPacket(CPacket* packet);

	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnRecvPacket(CPacket* packet) = 0;
	virtual void OnError(int errorCode, WCHAR* errorMessage) = 0;

	bool Start(const WCHAR* serverIp, uint16 port, uint32 concurrentThreadNum, uint32 workerThreadNum); // open ip, port, worker thread num, nagle option, maximum client num;
	void Stop();

public:
	bool isNetworkStop()
	{
		return _bStopNetwork;
	}

private:
	// iocp worker thread
	static unsigned __stdcall  WorkerThreadStatic(void* param)
	{
		CLanClient* thisClass = (CLanClient*)param;
		return thisClass->WorkerThread();
	}

	unsigned __stdcall WorkerThread();


	// monitoring thread
	static unsigned __stdcall MonitorThreadStatic(void* param)
	{
		CLanClient* thisClass = (CLanClient*)param;
		return thisClass->MonitorThread();
	}
	unsigned __stdcall MonitorThread();


	bool SendPost(Session* s);
	bool RecvPost(Session* s);
	void ReqWSASend(Session* session);




private:
	HANDLE _iocpHandle = NULL;
	bool _bStopNetwork = false;
	int _threadNumber = 0;
	HANDLE _threadList[100];
	uint16 _serverPort = 0;

	//monitoring value


public: //�α׿�
#define TPS_ARR_NUM 5


	int64 GetAcceptTPS()
	{
		static int acceptIndex = 0;
		return _monitoredAcceptTPS[acceptIndex++ % TPS_ARR_NUM];
	}

	int64 GetRecvMessageTPS()
	{
		static int recvIndex = 0;
		return _monitoredRecvTPS[recvIndex++ % TPS_ARR_NUM];
	}

	int64 GetSendMessageTPS()
	{
		static int sendIndex = 0;
		return _monitoredSendTPS[sendIndex++ % TPS_ARR_NUM];
	}


	int64 GetTotalAcceptValue()
	{
		return _totalAccept;
	}

	int64 GetTotalDisconnect()
	{
		return _totalDisconnet;
	}

	int64 _sessionNum;

private: // �α׿�
	int64 _monitoredSendTPS[TPS_ARR_NUM] = { 0, };
	int64 _monitoredRecvTPS[TPS_ARR_NUM] = { 0, };
	int64 _monitoredAcceptTPS[TPS_ARR_NUM] = { 0, };

	int64 _processSendPacket = 0;
	int64 _processRecvPacket = 0;
	int64 _acceptPacket = 0;


	int64 _totalDisconnet = 0;
	int64 _totalAccept = 0;

private: // ����
	Session* CreateSession(SOCKET socket);
	bool TryReleaseSession(Session* session, int line);
	bool ReleaseSession(Session* session);

private: // Ŭ�� ����
	Session* _clientSession = nullptr;
	SOCKET _socket;
	SOCKADDR_IN _serverAddr;



private: // ������ ����
	LockFreeQueue<Session*> _releaseSessionQueue;
	HANDLE hReleaseEvent = NULL;

private:
	void IncIoCount(Session* session);
	bool DecIoCount(Session* session);

private:
	uint16 _packeyKey = Data::clientPacketKey;
};

