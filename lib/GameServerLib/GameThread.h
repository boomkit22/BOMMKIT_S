#pragma once

#include "LockFreeQueue.h"
#include "Session.h"
#include "LockFreeStack.h"
#include <vector>
#include <map>

#define TPS_ARR_NUM 5
#define GAMETHREAD_MAX_SESSION_NUM 6000

struct MoveThreadInfo
{
	int64 sessionId;
	int threadFrom;
	int threadTo;
	void* moveInfo;
};

struct EnterSessionInfo
{
	int64 sessionId;
	void* ptr;
};

struct LeaveSessionInfo
{
	int64 sessionId;
	bool disconnect;
};

class CNetServer;
// 이것을 라이브러리 차원에서 어떻게하지
class CPacket;

extern std::map<int, class GameThread*> _gameThreadInfoMap;

class GameThread
{
	friend class CNetServer;

public:

	GameThread(int threadId, const int msPerFrame);

	int _msPerFrame;
	int _originalMsPerFrame;

	int64 GetFps()
	{
		static int sendIndex = 0;
		return _packetTps[sendIndex++ % TPS_ARR_NUM];
	}

	// 이 game thread 처리
private:

	virtual int64 GetPlayerSize() = 0;
	virtual void HandleRecvPacket(int64 sessionId, CPacket* packet) = 0;

	static unsigned __stdcall UpdateThreadStatic(void* param)
	{
		GameThread* thisClass = (GameThread*)param;
		return thisClass->UpdateThread();
	}
	unsigned __stdcall UpdateThread();
	HANDLE _hUpdateThread;

	static unsigned int __stdcall MonitorThreadStatic(void* arg)
	{
		GameThread* pThis = (GameThread*)arg;
		return pThis->MonitorThread();
		
	}

	unsigned int __stdcall MonitorThread();
	HANDLE _hMonitorThread;

	int64 _packetTps[TPS_ARR_NUM] = { 0, };
	int64 _updateTps = 0;

private:
	//TODO: fram 처리
	void NetworkRun();
	virtual void GameRun(float deltaTime) = 0;

private:
	bool EnterSession(int64 sessionId, void* ptr);
	void LeaveSession(int64 sessionId, bool disconnect);
	


public:
	//TODO: 게임쓰레드 이동 처리
	virtual void OnLeaveThread(int64 sessionId, bool disconnect) = 0;
	void SendPackets_Unicast(int64 sessionId, std::vector<CPacket*>& packets);
	void SendPacket_Unicast(int64 sessionId, CPacket* packet);
	bool MoveGameThread(int gameThreadId, int64 sessionId, void* ptr);


public:
	virtual void OnEnterThread(int64 sessionId, void* ptr) = 0;
	
public:
	void SetGameServer(CNetServer* server)
	{
		_netServer = server;
	}

private:
	CNetServer* _netServer = nullptr;

private: 
	LockFreeStack<uint16> _emptySessionIndex;
	SRWLOCK _sessionArrLock;
	//TODO: _sessionArr size 계속 체킄하기
	std::vector<int64> _sessionArr;
	LockFreeQueue<EnterSessionInfo> _enterQueue;
	LockFreeQueue<LeaveSessionInfo> _leaveQueue;

protected:
	void ProcessEnter();
	void ProcessLeave();

public:
	void Stop();
	bool CheckDead();
	void Kill();
	int64 GetSessionNum();
	std::vector<int64> GetSessions();


protected:
	uint16 _gameThreadID;
private:
	bool _running = true;
	bool _isAlive = true;
	int _runningThread = 0;
};

