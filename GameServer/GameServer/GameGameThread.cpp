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
		__debugbreak();//TODO: 악의적인 유저가 이상한 패킷을 보냈다-> 세션 끊는다
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
		// 이미 삭제된 경우
		// 더미 기준에서는 발생하면 안됨
		LOG(L"GameGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}
}


void GameGameThread::OnEnterThread(int64 sessionId, void* ptr)
{
	//TODO: map에 추가
	//TODO: 플레이어 생성
	Player* p = (Player*)ptr;
	auto result = _playerMap.insert({ sessionId, p });
	if (!result.second)
	{
		__debugbreak();
	}

	// 필드 이동 응답 보내고
	CPacket* packet = CPacket::Alloc();
	uint8 status = true;
	MP_SC_FIELD_MOVE(packet, status);
	//TODO: send
	SendPacket_Unicast(p->_sessionId, packet);
	printf("send field move\n");
	CPacket::Free(packet);

	// 내 캐릭터 소환 패킷 보내고
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

	//TODO: 다른 컈릭터들에게 이 캐릭터 소환 패킷 보내고
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

	//TODO: 이 캐릭터에게 이미 존재하고 있던 다른 캐릭터들 패킷 보내고
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


	//TODO: 몬스터들 소환 패킷 보내고 
}

void GameGameThread::HandleCharacterMove(Player* p, CPacket* packet)
{
	//TODO: 모든 유저에게 패킷 브로드캐스팅
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
	//브로드 캐스팅
	//TODO: 서버에서 검증하기
	
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
	//이건 플레이어 빼고 브로드캐스팅
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

