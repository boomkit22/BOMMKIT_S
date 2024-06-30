#include "Player.h"
using namespace std;


void Player::Init(int64 sessionId)
{
	_sessionId = sessionId;
	_lastRecvTime = 0;
	_bLogined = false;
	memset(id, 0, sizeof(id));
	memset(nickName, 0, sizeof(nickName));
}




