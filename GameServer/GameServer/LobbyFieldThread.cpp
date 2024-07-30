#include "LobbyFieldThread.h"
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
#include "GameServer.h"

using namespace std;

LobbyFieldThread::LobbyFieldThread(GameServer* gameServer, int threadId, int msPerFrame,
	uint16 sectorYLen, uint16 sectorXLen, uint16 sectorYSize, uint16 sectorXSize) : FieldPacketHandleThread(gameServer, threadId, msPerFrame, sectorYLen, sectorXLen, sectorYSize, sectorXSize)
{
	RegisterPacketHandler(PACKET_CS_GAME_REQ_CHARACTER_ATTACK, [this](Player* p, CPacket* packet) { HandleCharacterAttack(p, packet); });
}


void LobbyFieldThread::HandleCharacterAttack(Player* p, CPacket* packet)
{
	//브로드 캐스팅
	//TODO: 서버에서 검증하기

	int32 attackerType;
	int64 attackerID;
	int32 targetType;
	int64 targetID;

	*packet >> attackerType >> attackerID >> targetType >> targetID;
}

void LobbyFieldThread::GameRun(float deltaTime)
{
	UpdatePlayers(deltaTime);

	// Lobby Thread에는 몬스터 없고
	//UpdateMonsters(deltaTime);
}

void LobbyFieldThread::UpdatePlayers(float deltaTime)
{
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		Player* player = it->second;
		player->Update(deltaTime);
	}
}

void LobbyFieldThread::UpdateMonsters(float deltaTime)
{
	// Nothing
}

void LobbyFieldThread::OnEnterThread(int64 sessionId, void* ptr)
{
	//TODO: map에 추가
	//TODO: 플레이어 생성
	Player* p = (Player*)ptr;
	auto result = _playerMap.insert({ sessionId, p });
	if (!result.second)
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
	p->Position = spawnLocation;
	FRotator spawnRotation{ 0, 0, 0 };
	p->Rotation = spawnRotation;

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
		//spawnOtherCharacterInfo.NickName = p->NickName;
		printf("my playerinfo.hp = %d\n", p->playerInfo.Hp);
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, myPlayerInfo, spawnLocation, spawnRotation);
		SendPacket_Unicast(other->_sessionId, spawnOtherCharacterPacket);
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

		//spawnOtherCharacterInfo.NickName = p->NickName;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, otherPlayerInfo, OtherSpawnLocation, other->Rotation);
		SendPacket_Unicast(p->_sessionId, spawnOtherCharacterPacket);
		CPacket::Free(spawnOtherCharacterPacket);
	}
}


void LobbyFieldThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
	//TODO: 이 플레이어 despawn메시지 보내기
	auto it = _playerMap.find(sessionId);
	if (it == _playerMap.end())
	{
		LOG(L"GuardianFieldThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
		return;
	}
	Player* p = it->second;
	int64 characterNo = p->playerInfo.PlayerID;

	p->OnLeave();
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

	_fieldObjectMap.erase(characterNo);
}



