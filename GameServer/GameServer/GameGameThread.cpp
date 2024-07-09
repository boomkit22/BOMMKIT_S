#include "GameGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include <process.h>
#include "GameThreadInfo.h"
#include "Log.h"
#include "Packet.h"
using namespace std;


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


	default:
		LOG(L"Packet", LogLevel::Error, L"Packet Type Not Exist");
		__debugbreak();//TODO: �������� ������ �̻��� ��Ŷ�� ���´�-> ���� ���´�
	}

	CPacket::Free(packet);
}




void GameGameThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
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
	SendPacket_Unicast(p->_sessionId, packet);
	printf("send field move\n");
	CPacket::Free(packet);

	// �� ĳ���� ��ȯ ��Ŷ ������
	int spawnX = rand() % 400;
	int spawnY = rand() % 400;
	CPacket* spawnCharacterPacket = CPacket::Alloc();
	
	int64 PlayerID = p->playerID;
	FVector spawnLocation{ spawnX, spawnY, 100 };
	uint16 Level = p->Level;
	TCHAR NickName[20];
	wmemcpy(NickName, p->NickName, NICKNAME_LEN);
	p->Position = spawnLocation;

	
	MP_SC_SPAWN_MY_CHARACTER(spawnCharacterPacket, PlayerID, spawnLocation, Level, NickName);
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
		int64 PlayerID = p->playerID;
		FVector SpawnLocation2 = p->Position;
		uint16 Level = p->Level;
		TCHAR NickName[NICKNAME_LEN];
		wmemcpy(NickName, p->NickName, NICKNAME_LEN);

		printf("to other Spawn Location : %f, %f, %f\n", p->Position.X, p->Position.Y, p->Position.Z);
		//spawnOtherCharacterInfo.NickName = p->NickName;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, PlayerID, SpawnLocation2, Level, NickName);
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
		FVector SpawnLocation3 = other->Position;
		uint16 Level = other->Level;
		TCHAR NickName[NICKNAME_LEN];
		wmemcpy(NickName, other->NickName, NICKNAME_LEN);
		int PlayerId = other->playerID;

		printf("to me Spawn Location : %f, %f, %f\n", other->Position.X, other->Position.Y, other->Position.Z);

		//spawnOtherCharacterInfo.NickName = p->NickName;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, PlayerID, SpawnLocation3, Level, NickName);
		SendPacket_Unicast(p->_sessionId, spawnOtherCharacterPacket);
		printf("to me send spawn other character\n");
		CPacket::Free(spawnOtherCharacterPacket);
	}


	//TODO: ���͵� ��ȯ ��Ŷ ������ 
}

void GameGameThread::HandleCharacterMove(Player* p, CPacket* packet)
{
	//TODO: ��� �������� ��Ŷ ��ε�ĳ����
	int64 characterNo = p->playerID;
	FVector destination;
	*packet >> destination;

	CPacket* movePacket = CPacket::Alloc();
	MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, characterNo, destination);

	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		SendPacket_Unicast(it->first, movePacket);
	}
}

void GameGameThread::HandleCharacterAttack(Player* p, CPacket* packet)
{
	//��ε� ĳ����
	//TODO: �������� �����ϱ�
	
	int32 attackerType;
	int64 attackerID;
	int32 targetType;
	int64 targetID;
	int32 damage;

	*packet >> attackerType >> attackerID >> targetType >> targetID >> damage;

	 CPacket* resDamagePacket = CPacket::Alloc();
	 MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerID, targetType, targetID, damage);

	 for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	 {
		 printf("send attack to :%lld\n", it->first);
		 SendPacket_Unicast(it->first, resDamagePacket);
	 }
}

void GameGameThread::HandleCharacterSkill(Player* p, CPacket* packet)
{
	//�̰� �÷��̾� ���� ��ε�ĳ����
	int64 CharacterId = p->playerID;
	FRotator startRotation;
	int32 skillID;

	*packet >> startRotation >> skillID;

	CPacket* resSkillPacket = CPacket::Alloc();
	MP_SC_GAME_RES_CHARACTER_SKILL(resSkillPacket, CharacterId, startRotation, skillID);

	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		if (it->first == p->_sessionId)
			continue;
		SendPacket_Unicast(it->first, resSkillPacket);
	}
}

