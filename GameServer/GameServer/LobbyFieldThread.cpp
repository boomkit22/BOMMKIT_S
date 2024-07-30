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
	//��ε� ĳ����
	//TODO: �������� �����ϱ�

	int32 attackerType;
	int64 attackerID;
	int32 targetType;
	int64 targetID;

	*packet >> attackerType >> attackerID >> targetType >> targetID;
}

void LobbyFieldThread::GameRun(float deltaTime)
{
	UpdatePlayers(deltaTime);

	// Lobby Thread���� ���� ����
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
	//TODO: map�� �߰�
	//TODO: �÷��̾� ����
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

	FVector spawnLocation{ spawnX, spawnY,  PLAYER_Z_VALUE };
	p->Position = spawnLocation;
	FRotator spawnRotation{ 0, 0, 0 };
	p->Rotation = spawnRotation;

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
		//spawnOtherCharacterInfo.NickName = p->NickName;
		printf("my playerinfo.hp = %d\n", p->playerInfo.Hp);
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, myPlayerInfo, spawnLocation, spawnRotation);
		SendPacket_Unicast(other->_sessionId, spawnOtherCharacterPacket);
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

		//spawnOtherCharacterInfo.NickName = p->NickName;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, otherPlayerInfo, OtherSpawnLocation, other->Rotation);
		SendPacket_Unicast(p->_sessionId, spawnOtherCharacterPacket);
		CPacket::Free(spawnOtherCharacterPacket);
	}
}


void LobbyFieldThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
	//TODO: �� �÷��̾� despawn�޽��� ������
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
		// �̹� ������ ���
		// ���� ���ؿ����� �߻��ϸ� �ȵ�
		LOG(L"GuardianFieldThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}

	_fieldObjectMap.erase(characterNo);
}



