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
	SpiderFieldThread(GameServer* gameServer, int threadId, int msPerFrame);

private:
	virtual void UpdateMonsters(float deltaTime) override;

private:
	//∏ÛΩ∫≈Õ
	void SpawnMonster();
	int32 _maxMonsterNum = 100;
};
