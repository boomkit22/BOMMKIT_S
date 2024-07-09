#pragma once
#include "Session.h"
#include <unordered_map>
#include <algorithm>
#include "Type.h"
#include <map>

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

	int64 playerID; // 클라이언트에서 캐릭터 식별용


public:
	FVector Position;
	std::vector<PlayerInfo> playerInfos;
};

