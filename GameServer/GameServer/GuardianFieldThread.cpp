#include "GuardianFieldThread.h"
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

GuardianFieldThread::GuardianFieldThread(GameServer* gameServer,int threadId) : FieldPacketHandleThread(gameServer, threadId)
{
	RegisterPacketHandler(PACKET_CS_GAME_REQ_CHARACTER_ATTACK, [this](Player* p, CPacket* packet) { HandleCharacterAttack(p, packet); });
}

void GuardianFieldThread::OnEnterThread(int64 sessionId, void* ptr)
{
	Player* p = (Player*)ptr; 
	auto result = _playerMap.insert({ sessionId, p });
	if (!result.second)
	{
		__debugbreak();
	}
	p->StopMove();
	// �ʵ� �̵� ���� ������, �α��ξ����忡�� fieldID �ޱ��ϴµ� ������ ó���� lobby�ϰ�
	CPacket* packet = CPacket::Alloc();
	uint8 status = true;
	uint16 fieldID = _gameThreadID;
	MP_SC_FIELD_MOVE(packet, status, fieldID);
	//TODO: send
	SendPacket_Unicast(p->_sessionId, packet);
	printf("send field move\n");
	CPacket::Free(packet);

	// �� ĳ���� ��ȯ ��Ŷ ������
	int spawnX = MAP_SIZE_X / 2 + rand() % 300;
	int spawnY = MAP_SIZE_Y / 2 + rand() % 300;
	CPacket* spawnCharacterPacket = CPacket::Alloc();
	
	FVector spawnLocation{ spawnX, spawnY,  PLAYER_Z_VALUE};
	FRotator spawnRotation { 0, 0, 0 };
	p->Rotation = spawnRotation;
	p->Position = spawnLocation;

	PlayerInfo myPlayerInfo = p->playerInfo;
	MP_SC_SPAWN_MY_CHARACTER(spawnCharacterPacket, myPlayerInfo, spawnLocation, spawnRotation);
	SendPacket_Unicast(p->_sessionId, spawnCharacterPacket);
	printf("send spawn my character\n");
	CPacket::Free(spawnCharacterPacket);

	//TODO: �ٸ� �m���͵鿡�� �� ĳ���� ��ȯ ��Ŷ ������
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		if (it->first == sessionId)
			continue;
		Player* other = it->second;

		CPacket* spawnOtherCharacterPacket = CPacket::Alloc();
		printf("to other Spawn Location : %f, %f, %f\n", p->Position.X, p->Position.Y, p->Position.Z);
		//spawnOtherCharacterInfo.NickName = p->NickName;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, myPlayerInfo, spawnLocation, spawnRotation);
		SendPacket_Unicast(other->_sessionId, spawnOtherCharacterPacket);
		printf("to other send spawn other character\n");
		CPacket::Free(spawnOtherCharacterPacket);
	}

	//TODO: �� ĳ���Ϳ��� �̹� �����ϰ� �ִ� �ٸ� ĳ���͵� ��Ŷ ������
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		if (it->first == sessionId)
			continue;
		Player* other = it->second;

		CPacket* spawnOtherCharacterPacket = CPacket::Alloc();
		FVector OtherSpawnLocation = other->Position;
		PlayerInfo otherPlayerInfo = other->playerInfo;

		printf("to me Spawn Location : %f, %f, %f\n", other->Position.X, other->Position.Y, other->Position.Z);

		//spawnOtherCharacterInfo.NickName = p->NickName;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, otherPlayerInfo, OtherSpawnLocation, other->Rotation);
		SendPacket_Unicast(p->_sessionId, spawnOtherCharacterPacket);
		printf("to me send spawn other character\n");
		CPacket::Free(spawnOtherCharacterPacket);
	}


	//TODO: ���͵� ��ȯ ��Ŷ ������ 
	for (auto it = _monsters.begin(); it != _monsters.end(); it++)
	{
		Monster* monster = *it;
		CPacket* spawnMonsterPacket = CPacket::Alloc();
		MP_SC_SPAWN_MONSTER(spawnMonsterPacket ,monster->_monsterInfo, monster->_position, monster->_rotation);
		SendPacket_Unicast(p->_sessionId, spawnMonsterPacket);
		printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);
		CPacket::Free(spawnMonsterPacket);

		//���� �̵����̾����� �̵���Ŷ ���� ������
		if (monster->_state == MonsterState::MS_MOVING)
		{
			CPacket* movePacket = CPacket::Alloc();
			MP_SC_MONSTER_MOVE(movePacket, monster->_monsterInfo.MonsterID, monster->_destination, monster->_rotation);
			SendPacket_Unicast(p->_sessionId, movePacket);
			CPacket::Free(movePacket);
		}
	}
}


void GuardianFieldThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
	auto playerIt = _playerMap.find(sessionId);
	if (playerIt == _playerMap.end())
	{
		__debugbreak();
	}
	Player* player = playerIt->second;
	if (player == nullptr)
	{
		__debugbreak();
	}
	int64 characterNo = player->playerInfo.PlayerID;

	//���� ������ target���� �ϰ��ִ� player Empty���·� �����
	for (auto monster : _monsters)
	{
		if (monster->_targetPlayer == player)
		{
			monster->SetTargetPlayerEmpty();
		}
	}

	if (disconnect)
	{
		FreePlayer(sessionId);
	}
	else
	{
		LOG(L"GuardianFieldThread", LogLevel::Error, L"no disconnect : %lld, OnLeaveThread", sessionId);
	}

	int deletedNum = _playerMap.erase(sessionId);
	if (deletedNum == 0)
	{
		// �̹� ������ ���
		// ���� ���ؿ����� �߻��ϸ� �ȵ�
		LOG(L"GuardianFieldThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}

	CPacket* despawnPacket = CPacket::Alloc();
	MP_SC_GAME_DESPAWN_OTHER_CHARACTER(despawnPacket, characterNo);
	SendPacket_BroadCast(despawnPacket);
	CPacket::Free(despawnPacket);
}


void GuardianFieldThread::HandleCharacterAttack(Player* p, CPacket* packet)
{
	//��ε� ĳ����
	//TODO: �������� �����ϱ�
	int32 attackerType;
	int64 attackerID;
	int32 targetType;
	int64 targetID;

	*packet >> attackerType >> attackerID >> targetType >> targetID;
	
	int32 damage = p->_damage;

	
	if (targetType == TYPE_PLAYER)
	{
		//ĳ���� �ʿ��� ã�Ƽ�
		//ü�� ���
		auto it = _playerMap.find(targetID);
		if (it == _playerMap.end())
		{
			printf("Cannot find targetID : %lld, HandleCharacterAttack\n", targetID);
			return;
		}

		Player* targetPlayer = it->second;
		bool bDeath = targetPlayer->TakeDamage(damage);

		CPacket* resDamagePacket = CPacket::Alloc();
		MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerID, targetType, targetID, damage);
		SendPacket_BroadCast(resDamagePacket);
		CPacket::Free(resDamagePacket);

		if (bDeath)
		{
			//�׾����� ���� ��Ŷ���� ������
			CPacket* characterDeathPacket = CPacket::Alloc();
			MP_SC_GAME_RES_CHARACTER_DEATH(characterDeathPacket, targetID, p->Position, p->Rotation);
			SendPacket_BroadCast(characterDeathPacket);
			CPacket::Free(characterDeathPacket);
		}
	}
	
	//TODO: ���Ϳ��� �˻�
	 if (targetType == TYPE_MONSTER)
	 {
		 for (auto monster : _monsters)
		 {
			 if (monster->_monsterInfo.MonsterID == targetID)
			 {
				 if(monster->_state == MonsterState::MS_DEATH)
				 {
					 return;
				 }

				 CPacket* resDamagePacket = CPacket::Alloc();
				 MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerID, targetType, targetID, damage);
				 SendPacket_BroadCast(resDamagePacket);
				 CPacket::Free(resDamagePacket);
				 monster->TakeDamage(damage, p);
				 break;
			 }
		 }
	 }
}


void GuardianFieldThread::GameRun(float deltaTime)
{
	UpdatePlayers(deltaTime);
	UpdateMonsters(deltaTime);
}

void GuardianFieldThread::SpawnMonster()
{
	Monster*  monster = _monsterPool.Alloc();
	FVector randomLocation{ rand() % MAP_SIZE_X, rand() % MAP_SIZE_Y, 88.1 };
	FRotator spawnRotation = { 0, 0, 0 };
	monster->Init(this, randomLocation, MONSTER_TYPE_GUARDIAN);
	monster->_rotation = spawnRotation;

	std::clamp(randomLocation.X, double(100), double(MAP_SIZE_X - 100));
	std::clamp(randomLocation.Y, double(100), double(MAP_SIZE_Y - 100));

	_monsters.push_back(monster);

	//TODO: ���� ���� ��Ŷ ������
	CPacket* packet = CPacket::Alloc();
	MP_SC_SPAWN_MONSTER(packet, monster->_monsterInfo, randomLocation, spawnRotation);
	
	//printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);
	SendPacket_BroadCast(packet);
	CPacket::Free(packet);
}


void GuardianFieldThread::UpdatePlayers(float deltaTime)
{
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		Player* player = it->second;
		player->Update(deltaTime);
	}
}

void GuardianFieldThread::UpdateMonsters(float deltaTime)
{
	//TODO: ���� ���� Ȯ���ϱ�
	// ���� ������ Spawn �ϰ�
	int currentMonsterSize = _monsters.size();
	if (currentMonsterSize < _maxMonsterNum)
	{
		SpawnMonster();
		printf("Spawn Monster\n");
	}

	for (auto it = _monsters.begin(); it != _monsters.end(); it++)
	{
		//�׾����� �ϴ� Ǯ�� ����ְ�
		MonsterState state = (*it)->GetState();
		if (state == MonsterState::MS_DEATH)
		{
			_monsterPool.Free(*it);
			_monsters.erase(it);
			continue;
		}

		(*it)->Update(deltaTime);
	}
}