#pragma once
#include <map>
#include <vector>
#include "ObjectPool.h"
#include "FieldPacketHandleThread.h"


class Monster;
class Player;
class CPacket;

class GuardianFieldThread : public FieldPacketHandleThread
{
public:
	GuardianFieldThread(GameServer* gameServer, int threadId);

private:
	virtual void GameRun(float deltaTime) override;
	void UpdatePlayers(float deltaTime);
	void UpdateMonsters(float deltaTime);

	virtual void OnEnterThread(int64 sessionId, void* ptr) override;
	virtual void OnLeaveThread(int64 sessionId, bool disconnect) override;

private:
	//∏ÛΩ∫≈Õ
	std::vector<Monster*> _monsters;
	void SpawnMonster();
	CObjectPool<Monster, false> _monsterPool;
	int32 _maxMonsterNum = 100;
	
private:
	void HandleCharacterAttack(Player* p, CPacket* packet);
};

