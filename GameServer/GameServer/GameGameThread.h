#pragma once
#include "GameThread.h"
#include <map>
#include "Player.h"
#include "GameServer.h"
#include <vector>
#include "ObjectPool.h"
#include "Monster.h"

class GameGameThread : public GameThread
{
public:
	GameGameThread(GameServer* gameServer, int threadId);

public:
	int64 GetPlayerSize() override
	{
		return _playerMap.size();
	}

private:
	GameServer* _gameServer;
	std::unordered_map<int64, Player*> _playerMap;
	
private:
	//몬스터
	std::vector<Monster*> _monsters;
	virtual void GameRun(float deltaTime) override;
	void SpawnMonster();
	CObjectPool<Monster, false> _monsterPool;
	int32 _maxMonsterNum = 5;
	


public:
	// GameThread을(를) 통해 상속됨
	void HandleRecvPacket(int64 sessionId, CPacket* packet) override;
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;

	void SendPacket(int64 sessionId, CPacket* packet);
	void SendPacket_BroadCast(CPacket* packet);


private:
	void HandleCharacterMove(Player* p, CPacket* packet);
	void HandleCharacterAttack(Player* p, CPacket* packet);
	void HandleCharacterSkill(Player* p, CPacket* packet);


private:
	uint16 serverPacketCode = Data::serverPacketCode;


};

