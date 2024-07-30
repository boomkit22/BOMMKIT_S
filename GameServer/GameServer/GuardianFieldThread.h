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
	GuardianFieldThread(GameServer* gameServer, int threadId, int msPerFrame,
		uint16 sectorYLen, uint16 sectorXLen, uint16 sectorYSize, uint16 sectorXSize);


	virtual void FrameUpdate(float deltaTime) override;
private:
	//����
	void SpawnMonster();
	int32 _maxMonsterNum = 100;
};

