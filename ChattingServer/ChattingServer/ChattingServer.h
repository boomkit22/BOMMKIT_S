#pragma once
#include "CNetServer.h"
#include "LockFreeQueue.h"
#include "Job.h"
#include "Player.h"
#include "MonitorClient.h"
#include "PerformanceMonitor.h"


class ChattingServer : public CNetServer
{

public:
	ChattingServer();
	~ChattingServer();

private:
	void SendPacket_Unicast(Player* p, CPacket* packet);

private:
	void HandleTimeOut();
	void HandleAccept(int64 sessionId);
	void HandleDisconnect(int64 sessionId);
	void HandleLogin(int64 sessionId, CPacket* packet);
	void HandleHeartBeat(int64 sessinId, CPacket* packet);
	void HandleMessage(int64 sessionId, CPacket* packet);

private:
	// CLanServer��(��) ���� ��ӵ�
	virtual bool OnConnectionRequest() override;
	virtual void OnAccept(int64 sessionId) override;
	virtual void OnDisconnect(int64 sessionId) override;
	virtual void OnRecvPacket(int64 sessionId, CPacket* packet) override;
	virtual void OnError(int errorCode, WCHAR* errorMessage) override;

private:
	static unsigned int __stdcall UpdateThreadStatic(void* param)
	{
		ChattingServer* thisClass = (ChattingServer*)param;
		return thisClass->UpdateThread();
	}
	unsigned int __stdcall UpdateThread();

	static unsigned int __stdcall TimeOutThreadStatic(void* param)
	{
		ChattingServer* thisClass = (ChattingServer*)param;
		return thisClass->TimeOutThread();
	}
	unsigned int __stdcall TimeOutThread();

public:
	void LogServerInfo();
	void ObjectPoolLog();

private:
	HANDLE _hEvent;
	HANDLE _hWorkerThread;
	HANDLE _hTimeOutThread;

private:
	int64 _jobQueueSize;
	int64 _wakeUpCount;
	LockFreeQueue<Job> _packetQueue;

public:// ����� Ŭ���̾�Ʈ
	bool ActivateMonitorClient(const WCHAR* serverIp, uint16 port, uint32 concurrentThreadNum, uint32 workerThreadNum);

private:
	MonitorClient* _monitorClient = nullptr;

	static unsigned int __stdcall MonitorSendThreadStatic(void* arg)
	{
		ChattingServer* pThis = (ChattingServer*)arg;
		pThis->MonitorSendThread();
		return 0;
	}

	unsigned int __stdcall MonitorSendThread();

	static unsigned int __stdcall MonitorThreadStatic(void* arg)
	{
		ChattingServer* pThis = (ChattingServer*)arg;
		pThis->MonitorThread();
		return 0;
	}

	HANDLE _hMonitorThread;
	unsigned int __stdcall MonitorThread();

	HANDLE _hMonitorSendThread;
	bool _monitorActivated = false;

private: // ����� ������ ���� �� (����)
	int64 _loginTotal = 0;
	int64 _sectorMoveTotal = 0;
	int64 _messageTotal = 0;
	int64 _heartBeatTotal = 0;
	int64 _packetTps[TPS_ARR_NUM] = { 0, };
	int64 _updateTps = 0;
	int _processUserAllocMemory;
	int _networkSend;
	int _networkRecv;

	int64 GetUpdateTps()
	{
		static int sendIndex = 0;
		return _packetTps[sendIndex++ % TPS_ARR_NUM];
	}


private: // ����� ������ ���� �� (�ϵ����)
	PerformanceMonitor _performanceMonitor{ L"ChattingServer" };


private: // �÷��̾�
	std::unordered_map<int64, Player*> _playerMap;
	Player* GetPlayer(int64 sessionId);
	LockFreeObjectPool<class Player, false> _playerPool;

private:
	uint16 serverPacketCode = Data::serverPacketCode;
	uint16 clientPacketCode = Data::clientPacketCode;

	void MP_SC_CHAT_MESSAGE(CPacket* packet, int64& accountNo, WCHAR* id, WCHAR* nickName, uint16& messageLen, CPacket* message);
	void MP_SC_LOGIN(CPacket* packet, uint8& status, int64& accountNo);
	void MP_SC_SECTOR_MOVE(CPacket* packet, int64& accountNo, uint16& sectorX, uint16& sectorY);
	void MP_SC_MONITOR_TOOL_DATA_UPDATE(CPacket* packet, uint8& dataType, int& dataValue, int& timeStamp);
		void MP_SS_MONITOR_LOGIN(CPacket* packet, int& serverNo);

};


