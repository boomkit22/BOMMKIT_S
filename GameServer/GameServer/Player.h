#pragma once
#include "Session.h"
#include <unordered_map>
#include <algorithm>
#include "Type.h"
#include <map>

const double PLAYER_Z_VALUE = 95.2f;

class Player
{
	friend class GameServer;
	friend class BasePacketHandleThread;
	friend class GuardianFieldThread;
	friend class LobbyFieldThread;
	friend class SpiderFieldThread;
	friend class LoginGameThread;
	friend class FieldPacketHandleThread;

private:
	
	void Init(int64 sessionId);

private:
	int64 _sessionId = 0; // sessionid,  character id
	int64 accountNo = 0;
	uint32 _lastRecvTime = 0;
	bool _bLogined = false;
	int32 _damage = 10;


public:
	FVector Position;
	FRotator Rotation;
	PlayerInfo playerInfo;

	std::vector<PlayerInfo> playerInfos;
	void Update(float deltaTime);
	void SetDestination(const FVector& NewDestination);
	void StopMove();



private:
	FVector _destination;
	float _speed = 300.0f;
	bool bMoving = false;
	void Move(float deltaTime);


	/// <summary>
	/// 
	/// </summary>
	/// <param name="damage"></param>
	/// <returns> return true when death </returns>
	bool TakeDamage(int32 damage);
};

