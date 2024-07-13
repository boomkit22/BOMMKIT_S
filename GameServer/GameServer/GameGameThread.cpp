#include "GameGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include <process.h>
#include "GameThreadInfo.h"
#include "Log.h"
#include "Packet.h"
#include "GameData.h"
#include <algorithm>
#include "GamePacketMaker.h"

using namespace std;

//�̰� �������� ���ΰ� ���߿� ���Ͱ����Ǹ� ���ͷ� �ϸ� �Ǵϰ�


GameGameThread::GameGameThread(GameServer* gameServer,int threadId) : GameThread(threadId, 1)
{
	_gameServer = gameServer;
	SetGameServer((CNetServer*)gameServer);
}



void GameGameThread::HandleRecvPacket(int64 sessionId, CPacket * packet)
{
	Player* player = nullptr;
	auto it = _playerMap.find(sessionId);
	if (it == _playerMap.end())
	{
		LOG(L"GameGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, HandleRecvPacket", sessionId);
		return;
	}
	player = it->second;

	uint16 packetType;
	*packet >> packetType;

	switch (packetType)
	{

	case PACKET_CS_GAME_REQ_CHARACTER_MOVE:
	{
		HandleCharacterMove(player, packet);
	}
	break;

	case PACKET_CS_GAME_REQ_CHARACTER_ATTACK:
	{
		HandleCharacterAttack(player, packet);
	}
	break;

	case PACKET_CS_GAME_REQ_CHARACTER_SKILL:
	{
		HandleCharacterSkill(player, packet);
	}
	break;

	case PACKET_CS_GAME_REQ_CHARACTER_STOP:
	{
		HandleCharacterStop(player, packet);
	}
	break;



	default:
		LOG(L"Packet", LogLevel::Error, L"Packet Type Not Exist");
		__debugbreak();//TODO: �������� ������ �̻��� ��Ŷ�� ���´�-> ���� ���´�
	}

	CPacket::Free(packet);
}




void GameGameThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
	auto playerIt = _playerMap.find(sessionId);
	if (playerIt == _playerMap.end())
	{
		// �̹� ������ ���
		// ���� ���ؿ����� �߻��ϸ� �ȵ�
		__debugbreak();
	}
	Player* player = playerIt->second;
	if (player == nullptr)
	{
		__debugbreak();
	}

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
		_gameServer->FreePlayer(sessionId);
	}
	else
	{
		LOG(L"GameGameThread", LogLevel::Error, L"no disconnect : %lld, OnLeaveThread", sessionId);
	}

	int deletedNum = _playerMap.erase(sessionId);
	if (deletedNum == 0)
	{
		// �̹� ������ ���
		// ���� ���ؿ����� �߻��ϸ� �ȵ�
		LOG(L"GameGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}
}


void GameGameThread::OnEnterThread(int64 sessionId, void* ptr)
{
	//TODO: map�� �߰�
	//TODO: �÷��̾� ����
	Player* p = (Player*)ptr; 
	auto result = _playerMap.insert({ sessionId, p });
	if (!result.second)
	{
		__debugbreak();
	}

	// �ʵ� �̵� ���� ������
	CPacket* packet = CPacket::Alloc();
	uint8 status = true;
	MP_SC_FIELD_MOVE(packet, status);
	//TODO: send
	SendPacket(p->_sessionId, packet);
	printf("send field move\n");
	CPacket::Free(packet);

	// �� ĳ���� ��ȯ ��Ŷ ������
	int spawnX = rand() % 400;
	int spawnY = rand() % 400;
	CPacket* spawnCharacterPacket = CPacket::Alloc();
	
	FVector spawnLocation{ spawnX, spawnY, 100 };
	p->Position = spawnLocation;

	PlayerInfo myPlayerInfo = p->playerInfo;
	MP_SC_SPAWN_MY_CHARACTER(spawnCharacterPacket, myPlayerInfo, spawnLocation);
	SendPacket(p->_sessionId, spawnCharacterPacket);
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
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, myPlayerInfo, spawnLocation);
		SendPacket(other->_sessionId, spawnOtherCharacterPacket);
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
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, otherPlayerInfo, OtherSpawnLocation);
		SendPacket(p->_sessionId, spawnOtherCharacterPacket);
		printf("to me send spawn other character\n");
		CPacket::Free(spawnOtherCharacterPacket);
	}


	//TODO: ���͵� ��ȯ ��Ŷ ������ 
	for (auto it = _monsters.begin(); it != _monsters.end(); it++)
	{
		Monster* monster = *it;
		CPacket* spawnMonsterPacket = CPacket::Alloc();
		MP_SC_SPAWN_MONSTER(spawnMonsterPacket, (*it)->_monsterInfo, (*it)->_position);
		SendPacket(p->_sessionId, spawnMonsterPacket);
		printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);
		CPacket::Free(spawnMonsterPacket);

		if (monster->_state == MonsterState::MS_MOVING)
		{
			CPacket* movePacket = CPacket::Alloc();
			MP_SC_MONSTER_MOVE(movePacket, monster->_monsterInfo.MonsterID, monster->_destination, monster->_rotation);
			SendPacket(p->_sessionId, movePacket);
			CPacket::Free(movePacket);
		}

		//���� �̵����̾����� �̵���Ŷ ���� ������
	}
}

void GameGameThread::HandleCharacterMove(Player* p, CPacket* packet)
{
	//TODO: ��� �������� ��Ŷ ��ε�ĳ����
	int64 characterNo = p->playerInfo.PlayerID;
	FVector destination;
	FRotator startRotation;
	*packet >> destination >> startRotation;


	CPacket* movePacket = CPacket::Alloc();
	MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, characterNo, destination, startRotation);

	SendPacket_BroadCast(movePacket);
	CPacket::Free(movePacket);
}

void GameGameThread::HandleCharacterAttack(Player* p, CPacket* packet)
{
	//��ε� ĳ����
	//TODO: �������� �����ϱ�
	
	int32 attackerType;
	int64 attackerID;
	int32 targetType;
	int64 targetID;

	*packet >> attackerType >> attackerID >> targetType >> targetID;
	
	int32 damage = p->_damage;

	 CPacket* resDamagePacket = CPacket::Alloc();
	 MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerID, targetType, targetID, damage);
	 SendPacket_BroadCast(resDamagePacket);
	 CPacket::Free(resDamagePacket);



	 //TODO: ���Ϳ��� �˻�
	 if (targetType == TYPE_MONSTER)
	 {
		 for (auto monster : _monsters)
		 {
			 if (monster->_monsterInfo.MonsterID == targetID)
			 {
				 monster->TakeDamage(damage, p);
				 break;
			 }
		 }
	 }
}

void GameGameThread::HandleCharacterSkill(Player* p, CPacket* packet)
{
	//�̰� �÷��̾� ���� ��ε�ĳ����
	int64 CharacterId = p->playerInfo.PlayerID;
	FVector startLocation;
	FRotator startRotation;
	int32 skillID;

	*packet >> startLocation >> startRotation >> skillID;

	CPacket* resSkillPacket = CPacket::Alloc();
	MP_SC_GAME_RES_CHARACTER_SKILL(resSkillPacket, CharacterId, startLocation, startRotation, skillID);

	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		if (it->first == p->_sessionId)
			continue;
		SendPacket(it->first, resSkillPacket);
	}

	CPacket::Free(resSkillPacket);
}

void GameGameThread::HandleCharacterStop(Player* p, CPacket* packet)
{
	//��ε� ĳ����
	int64 characterID = p->playerInfo.PlayerID;
	FVector position;
	FRotator rotation;
	*packet >> position >> rotation;

	CPacket* stopPacket = CPacket::Alloc();
	MP_SC_GAME_RSE_CHARACTER_STOP(stopPacket, characterID, position, rotation);
	SendPacket_BroadCast(stopPacket);
	CPacket::Free(stopPacket);
}

void GameGameThread::GameRun(float deltaTime)
{
	//TODO: ���� ���� Ȯ���ϱ�
	// ���� ������ Spawn �ϰ�
	int currentMonsterSize = _monsters.size();
	if (currentMonsterSize < _maxMonsterNum)
	{
		SpawnMonster();
		printf("Spawn Monster\n");
	}

	for(auto it = _monsters.begin(); it != _monsters.end(); it++)
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

void GameGameThread::SpawnMonster()
{
	Monster*  monster = _monsterPool.Alloc();
	FVector randomLocation{ rand() % 2000, rand() % 2000, 88.1 };
	std::clamp(randomLocation.X, double(100), double(2000));
	std::clamp(randomLocation.Y, double(100), double(2000));

	monster->Init(this, randomLocation, MONSTER_TYPE_GUARDIAN);
	_monsters.push_back(monster);

	//TODO: ���� ���� ��Ŷ ������
	CPacket* packet = CPacket::Alloc();
	MP_SC_SPAWN_MONSTER(packet, monster->_monsterInfo, randomLocation);
	
	printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);

	SendPacket_BroadCast(packet);
	CPacket::Free(packet);
}

void GameGameThread::SendPacket(int64 sessionId, CPacket* packet)
{
	SendPacket_Unicast(sessionId, packet);
}

void GameGameThread::SendPacket_BroadCast(CPacket* packet)
{
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		SendPacket_Unicast(it->first, packet);
	}
}