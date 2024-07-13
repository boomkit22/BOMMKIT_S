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


public:
	FVector Position;
	PlayerInfo playerInfo;

	std::vector<PlayerInfo> playerInfos;
};

