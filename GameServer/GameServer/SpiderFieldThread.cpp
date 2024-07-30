#include "SpiderFieldThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include <process.h>
#include "Log.h"
#include "Packet.h"
#include "GameData.h"
#include <algorithm>
#include "PacketMaker.h"
#include "Monster.h"
#include "Player.h"

using namespace std;

SpiderFieldThread::SpiderFieldThread(GameServer* gameServer, int threadId, int msPerFrame,
	uint16 sectorYLen, uint16 sectorXLen, uint16 sectorYSize, uint16 sectorXSize)
	: FieldPacketHandleThread(gameServer, threadId, msPerFrame, sectorYLen, sectorXLen, sectorYSize, sectorXSize)
{
}


void SpiderFieldThread::FrameUpdate(float deltaTime)
{
	//TODO: 몬스터 갯수 확인하기
		// 몬스터 없으면 Spawn 하고
	int currentMonsterSize = _monsterMap.size();
	if (currentMonsterSize < _maxMonsterNum)
	{
		SpawnMonster();
		//printf("Spawn Monster\n");
	}

	for (auto it = _monsterMap.begin(); it != _monsterMap.end(); it++)
	{
		//죽었으면 일단 풀에 집어넣고
		MonsterState state = (*it).second->GetState();
		if (state == MonsterState::MS_DEATH)
		{
			ReturnFieldObject((*it).first);
		}
	}
}

void SpiderFieldThread::SpawnMonster()
{
	Monster* monster = AllocMonster(MONSTER_TYPE_SPIDER);

	FVector randomLocation{ rand() % MAP_SIZE_X, rand() % MAP_SIZE_Y, 88.1 };
	std::clamp(randomLocation.X, double(100), double(MAP_SIZE_X - 100));
	std::clamp(randomLocation.Y, double(100), double(MAP_SIZE_Y - 100));
	FRotator spawnRotation = { 0, 0, 0 };


	monster->_position = randomLocation;
	monster->_rotation = spawnRotation;

	monster->OnSpawn();
}

