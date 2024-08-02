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
	SpiderFieldThread(GameServer* gameServer, int threadId, int msPerFrame,
		uint16 sectorYLen, uint16 sectorXLen, uint16 sectorYSize, uint16 sectorXSize);

private:
	//∏ÛΩ∫≈Õ
	virtual void FrameUpdate(float deltaTime) override;
	void SpawnMonster();
	int32 _maxMonsterNum = 10000;
};
