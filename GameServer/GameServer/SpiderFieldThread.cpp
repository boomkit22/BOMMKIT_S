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

SpiderFieldThread::SpiderFieldThread(GameServer* gameServer, int threadId, int msPerFrame) : FieldPacketHandleThread(gameServer, threadId, msPerFrame)
{
}


void SpiderFieldThread::SpawnMonster()
{
	Monster* monster = _monsterPool.Alloc();
	FVector randomLocation{ rand() % MAP_SIZE_X, rand() % MAP_SIZE_Y, 88.1 };
	std::clamp(randomLocation.X, double(100), double(MAP_SIZE_X - 100));
	std::clamp(randomLocation.Y, double(100), double(MAP_SIZE_Y - 100));
	FRotator spawnRotation = { 0, 0, 0 };
	monster->Init(randomLocation, MONSTER_TYPE_SPIDER);
	monster->_rotation = spawnRotation;


	_monsters.push_back(monster);

	//TODO: 몬스터 스폰 패킷 날리기
	CPacket* packet = CPacket::Alloc();
	MP_SC_SPAWN_MONSTER(packet, monster->_monsterInfo, randomLocation, spawnRotation);

	//printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);

	SendPacket_BroadCast(packet);
	CPacket::Free(packet);
}

void SpiderFieldThread::UpdateMonsters(float deltaTime)
{

	//TODO: 몬스터 갯수 확인하기
// 몬스터 없으면 Spawn 하고
	int currentMonsterSize = _monsters.size();
	if (currentMonsterSize < _maxMonsterNum)
	{
		SpawnMonster();
		//printf("Spawn Monster\n");
	}

	for (auto it = _monsters.begin(); it != _monsters.end(); it++)
	{
		//죽었으면 일단 풀에 집어넣고
		MonsterState state = (*it)->GetState();
		if (state == MonsterState::MS_DEATH)
		{
			_monsterPool.Free(*it);
			_monsters.erase(it);
			continue;
		}

		vector<CPacket*> resPAckets = (*it)->Update(deltaTime);
		for (auto packet : resPAckets)
		{
			SendPacket_BroadCast(packet);
			CPacket::Free(packet);
		}
	}
}
