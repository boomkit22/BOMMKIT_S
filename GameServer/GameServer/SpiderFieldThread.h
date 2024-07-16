#pragma once
#include <map>
#include <vector>
#include "ObjectPool.h"
#include "FieldPacketHandleThread.h"

class Player;
class Monster;
class CPacket;

class SpiderFieldThread : public FieldPacketHandleThread
{
public:
	SpiderFieldThread(GameServer* gameServer, int threadId);

public:
	void UpdatePlayers(float deltaTime);
	void UpdateMonsters(float deltaTime);

private:
	//����
	std::vector<Monster*> _monsters;
	void SpawnMonster();
	CObjectPool<Monster, false> _monsterPool;
	int32 _maxMonsterNum = 50;

	
private:
	void HandleCharacterAttack(Player* p, CPacket* packet);

	// FieldPacketHandleThread��(��) ���� ��ӵ�
	void GameRun(float deltaTime) override;
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;
};
