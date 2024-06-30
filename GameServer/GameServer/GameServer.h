#pragma once
#include "CNetServer.h"
#include "LockFreeQueue.h"
#include "Player.h"
#include "MonitorClient.h"
#include "PerformanceMonitor.h"
#include "GameGameThread.h"
#include "LoginGameThread.h"
#include <map>
#include "Log.h"


class GameServer : public CNetServer
{

public:
	GameServer();
	~GameServer();

private:
	// CLanServer을(를) 통해 상속됨
	virtual bool OnConnectionRequest() override;
	virtual void OnError(int errorCode, WCHAR* errorMessage) override;

private:
	static unsigned int __stdcall TimeOutThreadStatic(void* param)
	{
		GameServer* thisClass = (GameServer*)param;
		return thisClass->TimeOutThread();
	}
	unsigned int __stdcall TimeOutThread();

public:
	void LogServerInfo();
	void ObjectPoolLog();

private:
	HANDLE _hTimeOutThread;



private:
	MonitorClient* _monitorClient = nullptr;

private: // 모니터 서버로 보낼 거 (서버)
	int64 _loginTotal = 0;
	int64 _sectorMoveTotal = 0;
	int64 _messageTotal = 0;
	int _processUserAllocMemory;
	int _networkSend;
	int _networkRecv;

private: // 모니터 서버로 보낼 거 (하드웨어)
	PerformanceMonitor _performanceMonitor{ L"GameServerLib" };

private: // 플레이어
	SRWLOCK _playerMapLock;
	LockFreeObjectPool<class Player, false> _playerPool;
	std::unordered_map<int64, Player*> _playerMap;
	GameGameThread* _gameGameThread = nullptr;
	LoginGameThread* _loginGameThread = nullptr;

public:
	Player* AllocPlayer(int64 sessionId)
	{
		Player* p = _playerPool.Alloc();
		AcquireSRWLockExclusive(&_playerMapLock);
		auto ret = _playerMap.insert({ sessionId, p });
		if (ret.second == false)
		{
			// already exist
			LOG(L"GameServer", LogLevel::Error, L"Already exist sessionId : %lld, AllocPlayer", sessionId);
			ReleaseSRWLockExclusive(&_playerMapLock);
			return nullptr;
		}
		ReleaseSRWLockExclusive(&_playerMapLock);
		return p;
	}

	void FreePlayer(int64 sessionId)
	{
		Player* p;
		AcquireSRWLockExclusive(&_playerMapLock);
		auto it = _playerMap.find(sessionId);
		if (it == _playerMap.end())
		{
			LOG(L"GameServer", LogLevel::Error, L"Cannot find sessionId : %lld, FreePlayer", sessionId);
			ReleaseSRWLockExclusive(&_playerMapLock);
			return;
		}
		p = (*it).second;
		_playerMap.erase(it);
		ReleaseSRWLockExclusive(&_playerMapLock);
		_playerPool.Free(p);
	}
	//Player* AllocPlayer()
	//{
	//	return _playerPool.Alloc();
	//}

	//void FreePlayer(Player* p)
	//{
	//	_playerPool.Free(p);
	//}
private:
	uint16 clientPacketCode = Data::clientPacketCode;
	void MP_SS_MONITOR_LOGIN(CPacket* packet, int& serverNo);
	void MP_SC_MONITOR_TOOL_DATA_UPDATE(CPacket* packet, uint8& dataType, int& dataValue, int& timeStamp);
};

