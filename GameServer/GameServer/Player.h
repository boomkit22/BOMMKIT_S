#pragma once
#include "Session.h"
#include <unordered_map>
#include <algorithm>
#include "Type.h"


class Player
{
	friend class GameServer;
	friend class GameGameThread;
	friend class LoginGameThread;

private:
	
	void Init(int64 sessionId);

private:
	int64 _sessionId = 0; // sessionid,  character id
	int64 accountNo = 0;
	uint32 _lastRecvTime = 0;
	bool _bLogined = false;

	TCHAR NickName[20];
	uint16 Level;
	FVector Postion;

	int64 playerID; // 클라이언트에서 캐릭터 식별용

};

