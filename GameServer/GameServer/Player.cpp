#include "Player.h"
#include "Type.h"

using namespace std;


void Player::Init(int64 sessionId)
{
	_sessionId = sessionId;
	_lastRecvTime = 0;
	_bLogined = false;
	playerInfo = PlayerInfo();
	Position = { 0, 0, 0 };
	playerInfos.clear();

}

