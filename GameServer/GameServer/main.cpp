#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "mysqlclient.lib")

#include <conio.h>
#include <stdio.h>
#include "Data.h"
#include <clocale>
#include "Type.h"
#include "GameServer.h"
#include "TlsObjectPool.h"
#include "Profiler.h"
#include "Log.h"


int main()
{
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	timeBeginPeriod(1);
	//������ �ε�
	bool loadDataSucceed = Data::LoadData();
	if (!loadDataSucceed)
	{
		wprintf(L"load data failed\n");
		return 0;
	}


	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		wprintf(L"wsa start up failed\n");
		return false;
	}


	// �α� �ʱ�ȭ 
	INIT_SYSLOG(L"Log"); //���� �̸� Log
	SYSLOG_LEVEL(LogLevel(Data::LogLevel));
	LOG(L"test", LogLevel::Error, L"error");

	//�������ϸ� �ʱ�ȭ
	ProfileInit();


	//ä�� ���� ����
	GameServer* gameServer = new GameServer;
	bool startSuccess = gameServer->Start(Data::ServerPort, Data::ServerConcurrentThreadNum,
		Data::ServerWorkerThreadNum, Data::Nagle, Data::SendZeroCopy);
	if (!startSuccess)
	{
		LOG(L"System", LogLevel::System, L"Start Chatting Server Failed");
		return 0;
	}
	LOG(L"System", LogLevel::System, L"Start Chatting Server Succeed");



	int logTime = timeGetTime();
	int logDuration = 1000;
	while (true)
	{
		Sleep(999);
		int now = timeGetTime();
		if (now - logTime > logDuration)
		{
			//1�ʴ� �α����
			//gameServer->LogServerInfo();
			// Packet TLSǮ �α����
			//gameServer->ObjectPoolLog();
			logTime += logDuration;
		}
		
		if (_kbhit())
		{
			//���� ����
			WCHAR controlKey = _getwch();
			if (L'q' == controlKey || L'Q' == controlKey)
			{
				LOG(L"System", LogLevel::System, L"Server Stop By Keyboard");
				gameServer->Stop();
				break;
			}else if (L'c' == controlKey || L'C' == controlKey)
			{
				// ���� �����
				LOG(L"System", LogLevel::System, L"Crash By Keyboard");
				__debugbreak();
			}
			else if (L's' == controlKey || L'S' == controlKey)
			{
				PRO_SAVE(L"profile");
			}
			else if (L'z' == controlKey || L'Z' == controlKey)
			{
				PRO_RESET();
			}
		}
	}

	delete gameServer;
	LOG(L"System", LogLevel::System, L"Main Thread Exit");
	WSACleanup();
	return 0;
}