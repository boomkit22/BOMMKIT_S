#define _CRT_SECURE_NO_WARNINGS

#include "LoginServer.h"
#include "SerializeBuffer.h"
#include "PacketHeader.h"
#include <process.h>
#include "TlsObjectPool.h"
#include "Packet.h"
#include "Profiler.h"
#include "LockFreeObjectPool.h"
#include <inttypes.h>
#include "MonitorPacketMaker.h"
#include "Log.h"
#include <algorithm>
#include <cpp_redis/cpp_redis>
#include "CpuUsage.h"
#include <Type.h>

using namespace std;

LoginServer::LoginServer()
{
	//���� �ʱ�ȭ
	// Player Array �ʱ�ȭ
	for (int i = 0; i < MAX_PLAYER_NUM; i++)
	{
		// �Ŀ� stack�� ���� index ����
		_playerArr[i]._index = i;
		// init lock
		InitializeSRWLock(&_playerArr[i]._lock);
	}

	// MAX_PLAYER_NUM���� ��� ���� player stack�� �ְ�
	for (int i = MAX_PLAYER_NUM - 1; i >= 0; i--)
	{
		_emptyPlayerIndex.Push(i);
	}

	// playerMap lock �ʱ�ȭ
	InitializeSRWLock(&_playerMapLock);
	int errorCode;

	_hTimeOutThread = (HANDLE)_beginthreadex(NULL, 0, TimeOutThreadStatic, this, 0, NULL);
	if (_hTimeOutThread == NULL)
	{
		errorCode = WSAGetLastError();
		LOG(L"System", LogLevel::System, L"Start TimeOut Thread Error");
		__debugbreak();
	}

	//InitMySQLConnection();
	//InitRedisConnection();

	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThreadStatic, this, 0, NULL);
	if (_hMonitorThread == NULL)
	{
		errorCode = WSAGetLastError();
		__debugbreak();
	}

	LOG(L"System", LogLevel::System, L"LoginServer()");
}


LoginServer::~LoginServer()
{

	WaitForSingleObject(_hTimeOutThread, INFINITE);
	//����� Ŭ�� �������¸�	
	if (_monitorActivated)
	{
		WaitForSingleObject(_hMonitorSendThread, INFINITE);
	}
	ClearMySQLonneection();
	ClearRedisConnection();

	LOG(L"System", LogLevel::System, L"~LoginServer()");
}

bool LoginServer::OnConnectionRequest()
{
	//virtual bool OnConnectionRequest(IP, Port) = 0; < accept ����
	/*return false; �� Ŭ���̾�Ʈ �ź�.
	return true; �� ���� ���*/

	//���ó : emptyPlayerIndex ��������� return false;
	//������ ȣ�� ����
	if (_emptyPlayerIndex.size() == 0)
	{
		return false;
	}

	return true;
}

void LoginServer::OnAccept(int64 sessionId)
{

}


void LoginServer::OnAccept(int64 sessionId, WCHAR* ip)
{
	printf("accept\n");
	LOG(L"Develop", LogLevel::Debug, L"Accept Client");
	
	PRO_BEGIN(L"OnAccept");
	uint16 playerIndex = MAX_PLAYER_NUM;
	bool popSucceed = _emptyPlayerIndex.Pop(playerIndex);
	if (!popSucceed)
	{
		//MAX_PLAYER_NUM ��ġ�� player�Ǵ��� Ȯ�� ( ���� )
		LOG(L"Accept", LogLevel::Error, L"Pop Empty Player Index Failed");
		__debugbreak();
		// ��ġ�� ����
		Disconnect(sessionId); 
	}

	//�÷��̾� �ʱ�ȭ ���ְ�
	Player* player = &_playerArr[playerIndex];
	AcquireSRWLockExclusive(&player->_lock);
	player->Init(sessionId, ip);
	ReleaseSRWLockExclusive(&player->_lock);

	//�ʿ� �־��ְ�
	AcquireSRWLockExclusive(&_playerMapLock);
	_playerMap.insert({ sessionId, player });
	ReleaseSRWLockExclusive(&_playerMapLock);
	PRO_END(L"OnAccept");
}

void LoginServer::OnDisconnect(int64 sessionId)
{
	printf("disconnect\n");
	LOG(L"Develop", LogLevel::Debug, L"Disconnect Client");
	PRO_BEGIN(L"OnDisconnect");
	Player* player = nullptr;
	//�ʿ��� ���� �����ϰ� index
	AcquireSRWLockExclusive(&_playerMapLock);
	auto it = _playerMap.find(sessionId);
	if (it == _playerMap.end())
	{
		__debugbreak();
	}
	player = (*it).second;
	_playerMap.erase(it);
	ReleaseSRWLockExclusive(&_playerMapLock);
	

	AcquireSRWLockExclusive(&player->_lock);
	// 1. ���� �������� OnDisconnect����
	// 2. player lock ��� �����ϰ� sessionId -1�� �ٲٸ�
	// 3. OnRecvPacket���� playerã�� ���·� �ִٰ� playerLock�� ��� sessionId Ȯ���ϸ� -1�̱� ������
	// 4. �߸��� �÷��̾� ��� ���� �� �ֹ�
	player->_sessionId = -1; 
	ReleaseSRWLockExclusive(&player->_lock);
	
	// ���� �� �� �ְ� player Index �ְ�
	_emptyPlayerIndex.Push(player->_index);
	PRO_END(L"OnDisconnect");
}


void LoginServer::OnRecvPacket(int64 sessionId, CPacket* packet)
{
	// �÷��̾� ã�Ƽ�
	// �÷��ƾ��� packetQueue���ֱ�
	Player* player;
	AcquireSRWLockShared(&_playerMapLock);
	auto it = _playerMap.find(sessionId);
	if (it == _playerMap.end())
	{
		__debugbreak();
	}
	player = (*it).second;
	ReleaseSRWLockShared(&_playerMapLock);
	
	//player�� ã�Ҵµ� lock�� ��Ҵ��� sessionId�� �ٲ������
	//�̹� ������ �����̴�
	AcquireSRWLockExclusive(&player->_lock);
	if (sessionId != player->_sessionId)
	{
		// �� ��� ������� Ȯ��
		// ���̰� ��û�� ���� ������ �������� ��û�� ������ ������ �� ��찡 �Ȼ���°� �´µ�
		// ����� ä�� ���� ���� �Ϲ����� ��Ȳ������ ���� �� �ִ�.
		InterlockedIncrement64(&_playerReuseCount);
		ReleaseSRWLockExclusive(&player->_lock);
		return;
	}

	PRO_BEGIN(L"Handle");
	HandleRecvPacket(player, packet);
	PRO_END(L"Handle");
	ReleaseSRWLockExclusive(&player->_lock);
}

void LoginServer::HandleRecvPacket(Player* player, CPacket* packet)
{
	uint16 packetType;
	*packet >> packetType;

	switch (packetType)
	{

	case PACKET_CS_LOGIN_REQ_LOGIN:
	{
		LOG(L"Develop", LogLevel::Debug, L"Login Recv");

		// �α���  �޽���
		InterlockedIncrement64(&_loginTotal);
		PRO_BEGIN(L"HandleLogin");
		HandleLogin(player, packet);
		PRO_END(L"HandleLogin");
	}
	break;

	case PACKET_CS_LOGIN_REQ_ECHO:
	{
		LOG(L"Develop", LogLevel::Debug, L"Echo Recv");
		HandleEcho(player, packet);
	}
	break;

	default:
		LOG(L"Packet", LogLevel::Error, L"Packet Type Not Exist");
		__debugbreak();//TODO: �������� ������ �̻��� ��Ŷ�� ���´�-> ���� ���´�
	}

	CPacket::Free(packet);
}

bool LoginServer::GetUserDataFromMysql(Player* p)
{
	//const char* query = "SELECT user_id, user_nick FROM account WHERE accountno : %d";	// From ���� DB�� �����ϴ� ���̺� ������ �����ϼ���
	
	int query_stat;
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	char query[100];
	sprintf(query, "SELECT userid, usernick FROM account WHERE accountno = %lld", p->accountNo);

	query_stat = mysql_query(_connections[_connectionIndex], query);
	if (query_stat != 0)
	{
		printf("Mysql query error : %s", mysql_error(&_conn[_connectionIndex]));
		return false;
	}

	sql_result = mysql_store_result(_connections[_connectionIndex]);		// ��� ��ü�� �̸� ������

	
	sql_row = mysql_fetch_row(sql_result);
	if (sql_row == NULL)
	{
		return false;
	}

	memcpy(p->id, sql_row[0], strlen(sql_row[0]));
	memcpy(p->nickName, sql_row[1], strlen(sql_row[1]));

		mysql_free_result(sql_result);
	return true;
}

void LoginServer::SetUserDataToRedis(Player* p)
{
	__int64 accountNo = p->accountNo;
	string key = std::to_string(p->accountNo);
	//_redis->setex(key, dfREDIS_TIMEOUT, user->_sessionKey);
	_redisClients[_connectionIndex].set(key, p->_sessionKey);
	_redisClients[_connectionIndex].sync_commit();

}

void LoginServer::GetIpForPlayer(Player* p, WCHAR* gameServerIp, uint16& gameServerPort, WCHAR* chatServerIp, uint16& chatServerPort)
{
	if (wmemcmp(p->_ip, L"10.0.1.2", wcslen(p->_ip)) == 0)
	{
		wmemcpy(gameServerIp, L"10.0.1.1", wcslen(L"10.0.1.1"));
		wmemcpy(chatServerIp, L"10.0.1.1", wcslen(L"10.0.1.1"));
		gameServerIp[wcslen(L"10.0.1.1")] = L'\0';
		chatServerIp[wcslen(L"10.0.1.1")] = L'\0';
	}
	else if (wmemcmp(p->_ip, L"10.0.2.2", wcslen(p->_ip)) == 0)
	{
		wmemcpy(gameServerIp, L"10.0.2.1", wcslen(L"10.0.2.1"));
		wmemcpy(chatServerIp, L"10.0.2.1", wcslen(L"10.0.2.1"));
		gameServerIp[wcslen(L"10.0.2.1")] = L'\0';
		chatServerIp[wcslen(L"10.0.2.1")] = L'\0';
	}
	else if (wmemcmp(p->_ip, L"127.0.0.1", wcslen(p->_ip)) == 0)
	{
		wmemcpy(gameServerIp, L"127.0.0.1", wcslen(L"127.0.0.1"));
		wmemcpy(chatServerIp, L"127.0.0.1", wcslen(L"127.0.0.1"));
		gameServerIp[wcslen(L"127.0.0.1")] = L'\0';
		chatServerIp[wcslen(L"127.0.0.1")] = L'\0';
	} 
	else {
		__debugbreak();
	}

	gameServerIp[wcslen(gameServerIp)] = L'\0';
	chatServerIp[wcslen(chatServerIp)] = L'\0';

	gameServerPort = _gameServerPort;
	chatServerPort = _chattingServerPort;

}


void LoginServer::HandleLogin(Player* player, CPacket* packet)
{
	InterlockedIncrement(&_loginTps);
	if (_connectionIndex == -1)
	{
		_connectionIndex = InterlockedIncrement(&connectionIndexCounter);
	}


	player->_lastRecvTime = timeGetTime();
	player->_bLogined = true; // ������ ���� ���µ�
	


	WCHAR gameServerIP[IP_LEN];
	uint16 gameServerPort;

	WCHAR chatServerIP[IP_LEN];
	uint16 chatServerPort;

	GetIpForPlayer(player, gameServerIP, gameServerPort, chatServerIP, chatServerPort);

	CPacket* resPacket = CPacket::Alloc();
	uint8 status = true;
	MP_SC_LOGIN(resPacket, player->accountNo, status, gameServerIP, gameServerPort, chatServerIP, chatServerPort);

	SendPacket_Unicast(player, resPacket);
	//printf("send login packet\n");

	CPacket::Free(resPacket);
}

void LoginServer::HandleEcho(Player* player, CPacket* packet)
{
	CPacket* resPacket = CPacket::Alloc();
	MP_SC_ECHO(resPacket);
	SendPacket_Unicast(player, resPacket);
	//printf("send echo packet\n");
	CPacket::Free(resPacket);

}


unsigned int __stdcall LoginServer::TimeOutThread()
{
	// PLAYER �迭�� �ѹ��� �� �˻���ϰ� ������ �˻������ν� timeout player�� �����ÿ�
	// Map lock ���� �Ŵ°� ����
	int order = 0;
	const int devidedNum = 4;
	const int loopTime = TIMEOUT_DISCONNECT / devidedNum;
	const int checkPlayerNumPerLoop = MAX_PLAYER_NUM / devidedNum;
	while (!isNetworkStop())
	{
		int checkNow = (order++) % devidedNum;
		Sleep(loopTime);
		//	uint32 timeNow = timeGetTime();
		//	for (int i = checkNow * checkPlayerNumPerLoop; i < (checkNow + 1) * checkPlayerNumPerLoop; i++)
		//	{
		//		Player* p = &_playerArr[i];
		//		AcquireSRWLockExclusive(&p->_lock);
		//		if (_playerArr[i]._sessionId <= 0)
		//		{
		//			ReleaseSRWLockExclusive(&p->_lock);
		//			continue;
		//		}

		//		uint32 playerLastRecvTime = p->_lastRecvTime;
		//		if (timeNow - playerLastRecvTime > TIMEOUT_DISCONNECT && timeNow > playerLastRecvTime)
		//		{
		//			//__debugbreak();
		//			_timeOutPlayerNum++;
		//			Disconnect(p->_sessionId);
		//		}
		//		ReleaseSRWLockExclusive(&p->_lock);
		//	}
		//}
	}
	return 0;
}


void LoginServer::SendPacket_Unicast(Player* p, CPacket* packet)
{
	SendPacket(p->_sessionId, packet);
}



void LoginServer::LogServerInfo()
{
	int64 totalAcceptValue = GetTotalAcceptValue();
	int64 acceptTPSValue = GetAcceptTPS();
	int64 sendMessageTPSValue = GetSendMessageTPS();
	int64 recvMessageTPSValue = GetRecvMessageTPS();
	int64 totalDisconnect = GetTotalDisconnect();
	int64 sessionNum = GetSessionNum();
	int64 disconnectBySendQueueFull = GetDisconnectSessionNumBySendQueueFull();
	int processUserAllocMemory = _processUserAllocMemory;
	int loginTps = GetLoginTps();
	int packetUseCount = (int)CPacket::GetUseCount();
	int networkSend = (int)GetNetworkSendByteTps() / 1000;
	int networkRecv = (int)GetNetworkRecvByteTps() / 1000;
	printf("Total Accept : %lld\n\
Accept TPS : % lld\n\
Send Message TPS : %lld\n\
Recv Message TPS : %lld\n\
session num :%lld\n\
total disconnect : %lld\n\
disconnect by sendqueue full : %lld\n\
memory : %d\n\
loginTps : %d\n\
packetuseCount : %d\n\
networkSend : %d\n\
networkRecv : %d\n\
================\n\
", totalAcceptValue, acceptTPSValue, sendMessageTPSValue, recvMessageTPSValue, sessionNum,
totalDisconnect, disconnectBySendQueueFull, processUserAllocMemory, loginTps, packetUseCount, networkSend, networkRecv);
}

void LoginServer::ObjectPoolLog()
{
	for (int i = 0; i < MAX_TLS_LOG_INDEX; i++)
	{
		if (_objectPoolMonitor[i].threadId == 0)
			break;

		TlsLog& tlsLog = _objectPoolMonitor[i];
		printf("thread id : %d , size = %d, mallocCount : %d\n", tlsLog.threadId, tlsLog.size, tlsLog.mallocCount);
	}

	printf("stack size : %d\n", releaseStackSize);
}

void LoginServer::OnError(int errorCode, WCHAR* errorMessage)
{
	//TODO: ���̺귯������ ������������ �޾Ƽ� �Ұ� �ϱ�
}




bool LoginServer::ActivateMonitorClient(const WCHAR* serverIp, uint16 port, uint32 concurrentThreadNum, uint32 workerThreadNum)
{
	_monitorClient = new MonitorClient();
	// iocp ����
	bool bMonitorStartSucceed = _monitorClient->Start(serverIp, port, concurrentThreadNum, workerThreadNum);
	if (!bMonitorStartSucceed)
		return false;
	// ����
	bool bConnectSucceed = _monitorClient->Connect();
	if (!bConnectSucceed)
		return false;

	int errorCode;
	_hMonitorSendThread = (HANDLE)_beginthreadex(NULL, 0, MonitorSendThreadStatic, this, 0, NULL);
	if (_hMonitorSendThread == NULL)
	{
		errorCode = WSAGetLastError();
		LOG(L"System", LogLevel::System, L"Create Monitor Send Thread Failed");
		__debugbreak();
	}

	_monitorActivated = true;
	return true;
}



unsigned int __stdcall LoginServer::MonitorSendThread()
{
	uint16 sessionNumPerSector[50][50];
	PerformanceMonitorData performanceMonitorData;
	CpuUsage cpuUsage;
	CPacket* loginPacket = CPacket::Alloc();
	int serverNo = LOGIN_SERVER_NO;

	MP_SS_MONITOR_LOGIN(loginPacket, serverNo);
	_monitorClient->SendPacket(loginPacket);
	CPacket::Free(loginPacket);

	Sleep(1000);
	while (!isNetworkStop())
	{
		//1�ʸ��� �ѹ��� ������ �� �� �ø���?
		Sleep(1000);


		//long long llTick;
		//time(&llTick);
		//int sendTick = (int)llTick;

		//_performanceMonitor.Update(performanceMonitorData);
		//cpuUsage.UpdateCpuTime();
		//int processorCpuTotal = cpuUsage.ProcessorTotal();

		////�α��μ��� ��Ŷ Ǯ ��뷮
		//int packetUseCount = (int)CPacket::GetUseCount();
		//CPacket* usecountpacket = CPacket::Alloc();
		//uint8 dataType = dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL;
		//MP_SC_MONITOR_TOOL_DATA_UPDATE(usecountpacket, dataType, packetUseCount, sendTick);
		//_monitorClient->SendPacket(usecountpacket);
		//CPacket::Free(usecountpacket);

		////�α��� ���� cpu
		//int processCpuTotal = cpuUsage.ProcessTotal();
		//CPacket* cpupacket = CPacket::Alloc();
		//dataType = dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU;
		//MP_SC_MONITOR_TOOL_DATA_UPDATE(cpupacket, dataType, processCpuTotal, sendTick);
		//_monitorClient->SendPacket(cpupacket);
		//CPacket::Free(cpupacket);

		//// �α��� ���� �޸�
		//int processUserAllocMemory = (int)performanceMonitorData.processUserAllocMemoryCounterVal.doubleValue / 1'000'000;
		//_processUserAllocMemory = processUserAllocMemory;
		//CPacket* mempacket = CPacket::Alloc();
		//dataType = dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM;
		//MP_SC_MONITOR_TOOL_DATA_UPDATE(mempacket, dataType, processUserAllocMemory, sendTick);
		//_monitorClient->SendPacket(mempacket);
		//CPacket::Free(mempacket);

		////�α��� ���� ���Ǽ�
		//int sessionNum = (int)GetSessionNum();
		//CPacket* sessionpacket = CPacket::Alloc();
		//dataType = dfMONITOR_DATA_TYPE_LOGIN_SESSION;
		//MP_SC_MONITOR_TOOL_DATA_UPDATE(sessionpacket, dataType, sessionNum, sendTick);
		//_monitorClient->SendPacket(sessionpacket);
		//CPacket::Free(sessionpacket);

		////�α��μ��� ����ó�� �ʴ� ó�� Ƚ��
		//int logintps = GetLoginTps();
		//CPacket* tpspacket = CPacket::Alloc();
		//dataType = dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS;
		//MP_SC_MONITOR_TOOL_DATA_UPDATE(tpspacket, dataType, logintps, sendTick);
		//_monitorClient->SendPacket(tpspacket);
		//CPacket::Free(tpspacket);



		////��Ʈ��ũ �۽�
		//int networkSend = (int)GetNetworkSendByteTps() / 1000;
		//CPacket* networkSendPacket = CPacket::Alloc();
		//dataType = dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS;
		//MP_SC_MONITOR_TOOL_DATA_UPDATE(networkSendPacket, dataType, networkSend, sendTick);
		//_monitorClient->SendPacket(networkSendPacket);
		//CPacket::Free(networkSendPacket);

		////��Ʈ��ũ ����
		//int networkRecv = (int)GetNetworkRecvByteTps() / 1000;
		//CPacket* networkRecvPacket = CPacket::Alloc();
		//dataType = dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG;
		//MP_SC_MONITOR_TOOL_DATA_UPDATE(networkRecvPacket, dataType, networkRecv, sendTick);
		//_monitorClient->SendPacket(networkRecvPacket);
		//CPacket::Free(networkRecvPacket);


	}

	return 0;
}

void LoginServer::InitMySQLConnection()
{
	for (int i = 0; i < MAX_CONNECTION_NUM; i++) {
		mysql_init(&_conn[i]);
		_connections[i] = mysql_real_connect(&_conn[i], host, user, password, database, port, NULL, 0);
		if (_connections[i] == NULL) {
			fprintf(stderr, "Mysql connection error (%d): %s\n", i, mysql_error(&_conn[i]));
			__debugbreak();
		}
	}

	LOG(L"System", LogLevel::System, L"InitMySQLConnection()");
}

void LoginServer::ClearMySQLonneection()
{
	for (int i = 0; i < MAX_CONNECTION_NUM; i++) {
		if (_connections[i] != NULL) {
			mysql_close(_connections[i]);
			printf("Connection %d closed.\n", i);
		}
	}
}

void LoginServer::InitRedisConnection()
{
	for (int i = 0; i < MAX_CONNECTION_NUM; ++i) {
		_redisClients[i].connect();

		if (_redisClients[i].is_connected()) {
		}
		else {
			__debugbreak();
		}
	}
	LOG(L"System", LogLevel::System, L"InitRedisConnection()");

}

void LoginServer::ClearRedisConnection()
{
	for (int i = 0; i < MAX_CONNECTION_NUM; ++i) {
		_redisClients[i].disconnect();
	}
}

unsigned int __stdcall LoginServer::MonitorThread()
{
	int tpsIndex = 0;
	while (true)
	{
		Sleep(1000);
		_loginTpsArr[tpsIndex % TPS_ARR_NUM] = _loginTps;
		_loginTps = 0;
		tpsIndex++;
	}

	return 0;
}

