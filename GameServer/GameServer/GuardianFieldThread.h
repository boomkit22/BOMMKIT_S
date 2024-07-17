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
	GuardianFieldThread(GameServer* gameServer, int threadId, int msPerFrame);

private:
	virtual void UpdateMonsters(float deltaTime) override;

private:
	//∏ÛΩ∫≈Õ
	void SpawnMonster();
	int32 _maxMonsterNum = 100;
};

