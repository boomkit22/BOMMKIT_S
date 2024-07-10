#include "GameServer.h"
#include "SerializeBuffer.h"
#include "PacketHeader.h"
#include <process.h>
#include "TlsObjectPool.h"
#include "Packet.h"
#include "Profiler.h"
#include "ObjectPool.h"
#include <inttypes.h>
#include "LockFreeObjectPool.h"
#include "TlsObjectPool.h"
#include "Log.h"
#include "MonitorPacketMaker.h"
#include "PerformanceMonitor.h"
#include "algorithm"
#include "GameGameThread.h"
#include "LoginGameThread.h"
#include "GameThreadInfo.h"
#include "CpuUsage.h"
#include <time.h>
//#define _LOG
//���������带 ������ �����?


using namespace std;
GameServer::GameServer()
{
	InitializeSRWLock(&_playerMapLock);
	_gameGameThread = new GameGameThread(this, ECHO_THREAD);
	_loginGameThread = new LoginGameThread(this, LOGIN_THREAD);
	RegisterDefaultGameThread(_loginGameThread);
	LOG(L"System", LogLevel::System, L"GameServer()");
}

GameServer::~GameServer()
{


	_gameGameThread->Stop();
	_loginGameThread->Stop();

	Sleep(1000);
	bool bEchoDead = _gameGameThread->CheckDead();
	if (!bEchoDead)
	{
		int64 sessionNum = _gameGameThread->GetSessionNum();
		LOG(L"EchoThread", LogLevel::Error, L"Session :%lld (num) is left", sessionNum);
		_gameGameThread->Kill();
	}
	
	bool bLoginDead = _loginGameThread->CheckDead();
	if(!bLoginDead)
	{
		int64 sessionNum = _loginGameThread->GetSessionNum();
		LOG(L"LoginThread", LogLevel::Error, L"Session :%lld (num) is left", sessionNum);
		_loginGameThread->Kill();
	}

	delete _gameGameThread;
	delete _loginGameThread;
	LOG(L"System", LogLevel::System, L"~GameServer()");
}

unsigned int __stdcall GameServer::TimeOutThread()
{
	while (true)
	{
		Sleep(1000);
	}

	return 0;
}

void GameServer::LogServerInfo()
{
	int64 totalAcceptValue = GetTotalAcceptValue();
	int64 acceptTPSValue = GetAcceptTPS();
	int64 sendMessageTPSValue = GetSendMessageTPS();
	int64 recvMessageTPSValue = GetRecvMessageTPS();
	int64 totalDisconnect = GetTotalDisconnect();
	int64 sessionNum = GetSessionNum();
	int64 disconnectBySendQueueFull = GetDisconnectSessionNumBySendQueueFull();
	int processUserAllocMemory = _processUserAllocMemory;
	int networkSend = _networkSend;
	int networkRecv = _networkRecv;

	int loginPlayerNum = _loginGameThread->GetPlayerSize();
	int gamePlayerNum = _gameGameThread->GetPlayerSize();
	int64 totalPlayerNum = _playerMap.size();
	int loginTps = _loginGameThread->GetFps();
	int echoTps = _gameGameThread->GetFps();

	printf("Total Accept : %lld\n\
Accept TPS : % lld\n\
Send Message TPS : %lld\n\
Recv Message TPS : %lld\n\
session num :%lld\n\
login : %d\n\
game: %d\n\
total: %lld\n\
total disconnect : %lld\n\
disconnect by sendqueue full : %lld\n\
memory : %d\n\
send: %d\n\
recv: %d\n\
logintTps : %d\n\
echoTps : %d\n\
================\n\
", totalAcceptValue, acceptTPSValue, sendMessageTPSValue, recvMessageTPSValue, sessionNum, loginPlayerNum, gamePlayerNum, totalPlayerNum,
totalDisconnect, disconnectBySendQueueFull, processUserAllocMemory, networkSend, networkRecv, loginTps, echoTps);
}


void GameServer::ObjectPoolLog()
{
	printf("packetusecount : %lld\n", CPacket::GetUseCount());
	printf("PlayerPool : capaicty : %d useCount : %d\n", _playerPool.GetCapacityCount(), _playerPool.GetUseCount());
	for (int i = 0; i < MAX_TLS_LOG_INDEX; i++)
	{
		if (_objectPoolMonitor[i].threadId == 0)
			break;

		TlsLog& tlsLog = _objectPoolMonitor[i];


		printf("thread id : %d , size = %d, mallocCount : %d\n", tlsLog.threadId, tlsLog.size, tlsLog.mallocCount);
	}

	printf("stack size : %d\n", releaseStackSize);
}




bool GameServer::OnConnectionRequest()
{
	// �̰� GameServer
	return false;
}

void GameServer::OnError(int errorCode, WCHAR* errorMessage)
{
	// �̰Ŵ� ??? 
}
