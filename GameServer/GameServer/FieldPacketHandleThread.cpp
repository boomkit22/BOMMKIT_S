#include "FieldPacketHandleThread.h"
#include "Packet.h"
#include "SerializeBuffer.h"
#include "PacketMaker.h"
#include "Player.h"
#include "GameServer.h"
#include "Monster.h"
#include "GameData.h"

FieldPacketHandleThread::FieldPacketHandleThread(GameServer* gameServer, int threadId, int msPerFrame) : BasePacketHandleThread(gameServer, threadId, msPerFrame)
{
	RegisterPacketHandler(PACKET_CS_GAME_REQ_FIELD_MOVE, [this](Player* p, CPacket* packet) { HandleFieldMove(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_CHARACTER_MOVE, [this](Player* p, CPacket* packet) { HandleChracterMove(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_CHARACTER_SKILL, [this](Player* p, CPacket* packet) { HandleCharacterSkill(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_CHARACTER_STOP, [this](Player* p, CPacket* packet) { HandleCharacterStop(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_CHARACTER_ATTACK, [this](Player* p, CPacket* packet) { HnadleCharacterAttack(p, packet); });
}

void FieldPacketHandleThread::HandleFieldMove(Player* player, CPacket* packet)
{
	uint16 fieldID;
	*packet >> fieldID;
	MoveGameThread(fieldID, player->_sessionId, player);
}

void FieldPacketHandleThread::HandleChracterMove(Player* player, CPacket* packet)
{
	int64 characterNo = player->playerInfo.PlayerID;
	FVector destination;
	FRotator startRotation;
	*packet >> destination >> startRotation;

	CPacket* movePacket = CPacket::Alloc();
	MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, characterNo, destination, startRotation);

	//브로드케스팅
	SendPacket_BroadCast(movePacket);
	CPacket::Free(movePacket);
	player->SetDestination(destination);
}

void FieldPacketHandleThread::HandleCharacterSkill(Player* player, CPacket* packet)
{
	int64 CharacterId = player->playerInfo.PlayerID;
	FVector startLocation;
	FRotator startRotation;
	int32 skillID;

	*packet >> startLocation >> startRotation >> skillID;

	CPacket* resSkillPacket = CPacket::Alloc();
	MP_SC_GAME_RES_CHARACTER_SKILL(resSkillPacket, CharacterId, startLocation, startRotation, skillID);

	// 이 플레이어 뺴고 브로드캐스팅
	SendPacket_BroadCast(resSkillPacket, player);
	CPacket::Free(resSkillPacket);
}

void FieldPacketHandleThread::HandleCharacterStop(Player* player, CPacket* packet)
{
	int64 characterID = player->playerInfo.PlayerID;
	FVector position;
	FRotator rotation;
	*packet >> position >> rotation;

	CPacket* stopPacket = CPacket::Alloc();
	MP_SC_GAME_RSE_CHARACTER_STOP(stopPacket, characterID, position, rotation);

	//브로드 캐스팅
	SendPacket_BroadCast(stopPacket);
	CPacket::Free(stopPacket);
}

void FieldPacketHandleThread::HnadleCharacterAttack(Player* player, CPacket* packet)
{
	//브로드 캐스팅
	//TODO: 서버에서 검증하기
	int32 attackerType;
	int64 attackerID;
	int32 targetType;
	int64 targetID;

	*packet >> attackerType >> attackerID >> targetType >> targetID;

	int32 damage = player->_damage;
	
	


	if (targetType == TYPE_PLAYER)
	{
		printf("HandleCharacterAttack to player\n");

		//캐릭터 맵에서 찾아서
		//체력 깎고
		auto targetIt = _playerIDToPlayerMap.find(targetID);
		if (targetIt == _playerIDToPlayerMap.end())
		{
			printf("Cannot find targetID : %lld, HandleCharacterAttack\n", targetID);
			return;
		}

		Player* targetPlayer = targetIt->second;
		bool bDeath = targetPlayer->TakeDamage(damage);

		printf("targetPlayer->TakeDamage\n");
		CPacket* resDamagePacket = CPacket::Alloc();
		MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerID, targetType, targetID, damage);
		SendPacket_BroadCast(resDamagePacket);
		CPacket::Free(resDamagePacket);

		if (bDeath)
		{
			//죽었으면 죽은 패킷까지 보내고
			CPacket* characterDeathPacket = CPacket::Alloc();
			MP_SC_GAME_RES_CHARACTER_DEATH(characterDeathPacket, targetID, player->Position, player->Rotation);
			SendPacket_BroadCast(characterDeathPacket);
			CPacket::Free(characterDeathPacket);
		}
	}

	//TODO: 몬스터에서 검색
	if (targetType == TYPE_MONSTER)
	{
		for (auto monster : _monsters)
		{
			if (monster->_monsterInfo.MonsterID == targetID)
			{
				if (monster->_state == MonsterState::MS_DEATH)
				{
					return;
				}

				CPacket* resDamagePacket = CPacket::Alloc();
				MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerID, targetType, targetID, damage);
				SendPacket_BroadCast(resDamagePacket);
				CPacket::Free(resDamagePacket);


				bool bDeath = monster->TakeDamage(damage, player);
				if (bDeath)
				{
					CPacket* monsterDeathPacket = CPacket::Alloc();
					MP_SC_GAME_RES_MONSTER_DEATH(monsterDeathPacket, targetID, monster->_position, monster->_rotation);
					SendPacket_BroadCast(monsterDeathPacket);
					CPacket::Free(monsterDeathPacket);
				}

				break;
			}
		}
	}
}

void FieldPacketHandleThread::GameRun(float deltaTime)
{
	UpdatePlayers(deltaTime);
	UpdateMonsters(deltaTime);
}

void FieldPacketHandleThread::UpdatePlayers(float deltaTime)
{
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		Player* player = it->second;
		player->Update(deltaTime);
	}
}

void FieldPacketHandleThread::OnEnterThread(int64 sessionId, void* ptr)
{
	Player* p = (Player*)ptr;
	auto result = _playerMap.insert({ sessionId, p });
	if (!result.second)
	{
		__debugbreak();
	}
	auto result2 = _playerIDToPlayerMap.insert({ p->playerInfo.PlayerID, p });
	if (!result2.second)
	{
		__debugbreak();
	}


	p->StopMove();
	// 필드 이동 응답 보내고, 로그인쓰레드에서 fieldID 받긴하는데 어차피 처음엔 lobby니가
	CPacket* packet = CPacket::Alloc();
	uint8 status = true;
	uint16 fieldID = _gameThreadID;
	MP_SC_FIELD_MOVE(packet, status, fieldID);
	//TODO: send
	SendPacket_Unicast(p->_sessionId, packet);
	printf("send field move\n");
	CPacket::Free(packet);

	// 내 캐릭터 소환 패킷 보내고
	int spawnX = MAP_SIZE_X / 2 + rand() % 300;
	int spawnY = MAP_SIZE_Y / 2 + rand() % 300;
	CPacket* spawnCharacterPacket = CPacket::Alloc();

	FVector spawnLocation{ spawnX, spawnY,  PLAYER_Z_VALUE };
	FRotator spawnRotation{ 0, 0, 0 };
	p->Rotation = spawnRotation;
	p->Position = spawnLocation;

	PlayerInfo myPlayerInfo = p->playerInfo;
	MP_SC_SPAWN_MY_CHARACTER(spawnCharacterPacket, myPlayerInfo, spawnLocation, spawnRotation);
	SendPacket_Unicast(p->_sessionId, spawnCharacterPacket);
	printf("send spawn my character\n");
	CPacket::Free(spawnCharacterPacket);

	//TODO: 다른 컈릭터들에게 이 캐릭터 소환 패킷 보내고
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

	//TODO: 이 캐릭터에게 이미 존재하고 있던 다른 캐릭터들 패킷 보내고
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


	//TODO: 몬스터들 소환 패킷 보내고 
	for (auto it = _monsters.begin(); it != _monsters.end(); it++)
	{
		Monster* monster = *it;
		CPacket* spawnMonsterPacket = CPacket::Alloc();
		MP_SC_SPAWN_MONSTER(spawnMonsterPacket, monster->_monsterInfo, monster->_position, monster->_rotation);
		SendPacket_Unicast(p->_sessionId, spawnMonsterPacket);
		//printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);
		CPacket::Free(spawnMonsterPacket);

		//현재 이동중이었으면 이동패킷 까지 보내기
		if (monster->_state == MonsterState::MS_MOVING)
		{
			CPacket* movePacket = CPacket::Alloc();
			MP_SC_MONSTER_MOVE(movePacket, monster->_monsterInfo.MonsterID, monster->_destination, monster->_rotation);
			SendPacket_Unicast(p->_sessionId, movePacket);
			CPacket::Free(movePacket);
		}
	}
}

void FieldPacketHandleThread::OnLeaveThread(int64 sessionId, bool disconnect)
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
	int64 playerID = player->playerInfo.PlayerID;

	//나간 유저를 target으로 하고있던 player Empty상태로 만들기
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
		// 이미 삭제된 경우
		// 더미 기준에서는 발생하면 안됨
		LOG(L"GuardianFieldThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}

	_playerIDToPlayerMap.erase(playerID);

	CPacket* despawnPacket = CPacket::Alloc();
	MP_SC_GAME_DESPAWN_OTHER_CHARACTER(despawnPacket, playerID);
	SendPacket_BroadCast(despawnPacket);
	CPacket::Free(despawnPacket);
}





