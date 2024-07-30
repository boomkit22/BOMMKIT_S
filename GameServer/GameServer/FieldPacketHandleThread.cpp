#include "FieldPacketHandleThread.h"
#include "Packet.h"
#include "SerializeBuffer.h"
#include "PacketMaker.h"
#include "Player.h"
#include "GameServer.h"
#include "Monster.h"
#include "GameData.h"
#include "Sector.h"
using namespace std;

FieldPacketHandleThread::FieldPacketHandleThread(GameServer* gameServer, int threadId, int msPerFrame,
	uint16 _sectorYLen, uint16 _sectorXLen, uint16 _sectorYSize, uint16 _sectorXSize) :
	BasePacketHandleThread(gameServer, threadId, msPerFrame), _sectorYLen(_sectorYLen), _sectorXLen(_sectorXLen), _sectorYSize(_sectorYSize), _sectorXSize(_sectorXSize)
{
	InitializeSector();
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
	
	MoveGameThread(fieldID, player->GetObjectId(), player);
}

void FieldPacketHandleThread::HandleChracterMove(Player* player, CPacket* packet)
{
	int64 characterNo = player->playerInfo.PlayerID;
	FVector destination;
	FRotator startRotation;
	*packet >> destination >> startRotation;

	player->HandleCharacterMove(destination, startRotation);
}

void FieldPacketHandleThread::HandleCharacterSkill(Player* player, CPacket* packet)
{
	int64 CharacterId = player->playerInfo.PlayerID;
	FVector startLocation;
	FRotator startRotation;
	int32 skillID;
	*packet >> startLocation >> startRotation >> skillID;
	
	player->HandleCharacterSkill(startLocation, startRotation, skillID);
}

void FieldPacketHandleThread::HandleCharacterStop(Player* player, CPacket* packet)
{
	int64 characterID = player->playerInfo.PlayerID;
	FVector position;
	FRotator rotation;
	*packet >> position >> rotation;

	player->HandleCharacterStop(position, rotation);
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

	player->HandleCharacterAttack(attackerType, attackerID, targetType, targetID);

	
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
	auto result2 = _fieldObjectMap.insert({ p->GetObjectId(), p });
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

FieldObject* FieldPacketHandleThread::FindFieldObject(int64 objectId)
{
	auto it = _fieldObjectMap.find(objectId);
	
	if (it != _fieldObjectMap.end())
	{
		return it->second;
	}

	return nullptr;
}

void FieldPacketHandleThread::InitializeSector()
{
	const CHAR dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
	const CHAR dx[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

	_sector = new Sector*[_sectorYLen];
	for(int i = 0; i < _sectorYLen; i++)
	{
		_sector[i] = new Sector[_sectorXLen];
	}

	for (int y = 0; y < _sectorYLen; y++)
	{
		for (int x = 0; x < _sectorXLen; x++)
		{
			_sector[y][x].X = x;
			_sector[y][x].Y = y;
		}
	}
	// 양옆 조건 문제없는 가운데부분먼저
	for (int y = 1; y < _sectorYLen - 1; y++)
	{
		for (int x = 1; x < _sectorXLen - 1; x++)
		{
			// around 설정
			_sector[y][x].aroundSectorNum = 9;
			for (int i = 0; i < MOVE_DIR_MAX; i++)
			{
				_sector[y][x]._around[i] = &_sector[y + dy[i]][x + dx[i]];
			}
			_sector[y][x]._around[MOVE_DIR_MAX] = &_sector[y][x];

			// 왼쪽 이동 설정
			_sector[y][x].leftSectorNum = 3;
			_sector[y][x].left[0] = &_sector[y + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
			_sector[y][x].left[1] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].left[2] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];

			// 오른쪽 이동 설정
			_sector[y][x].rightSectorNum = 3;
			_sector[y][x].right[0] = &_sector[y + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
			_sector[y][x].right[1] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].right[2] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];

			// 위쪽 이동 설정
			_sector[y][x].upSectorNum = 3;
			_sector[y][x].up[0] = &_sector[y + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
			_sector[y][x].up[1] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].up[2] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];

			//아래 이동 설정
			_sector[y][x].downSectorNum = 3;
			_sector[y][x].down[0] = &_sector[y + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
			_sector[y][x].down[1] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
			_sector[y][x].down[2] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];

			// 위 오른쪽 이동 설정
			_sector[y][x].upRightSectorNum = 5;
			_sector[y][x].upRight[0] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].upRight[1] = &_sector[y + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
			_sector[y][x].upRight[2] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].upRight[3] = &_sector[y + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
			_sector[y][x].upRight[4] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];

			// 위 왼쪽 이동 설정
			_sector[y][x].upLeftSectorNum = 5;
			_sector[y][x].upLeft[0] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].upLeft[1] = &_sector[y + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
			_sector[y][x].upLeft[2] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].upLeft[3] = &_sector[y + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
			_sector[y][x].upLeft[4] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];

			// 아래 오른쪽 이동 설정
			_sector[y][x].downRightSectorNum = 5;
			_sector[y][x].downRight[0] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].downRight[1] = &_sector[y + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
			_sector[y][x].downRight[2] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
			_sector[y][x].downRight[3] = &_sector[y + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
			_sector[y][x].downRight[4] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];

			// 아래 왼쪽 이동 설정
			_sector[y][x].downLeftSectorNum = 5;
			_sector[y][x].downLeft[0] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].downLeft[1] = &_sector[y + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
			_sector[y][x].downLeft[2] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
			_sector[y][x].downLeft[3] = &_sector[y + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
			_sector[y][x].downLeft[4] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		}
	}

	// 맨위 y = 0 설정
	for (int x = 1; x < _sectorXLen - 1; x++)
	{
		// 주변 설정
		_sector[0][x].aroundSectorNum = 6;
		_sector[0][x]._around[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x]._around[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		_sector[0][x]._around[2] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x]._around[3] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		_sector[0][x]._around[4] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x]._around[5] = &_sector[0][x]; //자기자신

		// 왼쪽 이동 설정
		_sector[0][x].leftSectorNum = 2;
		_sector[0][x].left[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x].left[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		// 오른쪽 이동 설정
		_sector[0][x].rightSectorNum = 2;
		_sector[0][x].right[0] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x].right[1] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		// 위쪽 이동 설정
		_sector[0][x].upSectorNum = 0;
		// 아래 이동 설정
		/* 아래로 와서 이쪽으로 올 수는 없는데 일단 만들기*/
		_sector[0][x].downSectorNum = 3;
		_sector[0][x].down[0] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		_sector[0][x].down[1] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x].down[2] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		// 위 오른족 이동 설정
		_sector[0][x].upRightSectorNum = 2;
		_sector[0][x].upRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x].upRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		// 위 왼쪽 이동 설정
		_sector[0][x].upLeftSectorNum = 2;
		_sector[0][x].upLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x].upLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		// 아래 오른쪽 이동 설정
		/* 아래로 와서 이쪽으로 올 수는 없는데 일단 만들기*/
		_sector[0][x].downRightSectorNum = 4;
		_sector[0][x].downRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x].downRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		_sector[0][x].downRight[2] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x].downRight[3] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		//아래 왼쪽 이동 설정
		/* 아래로 와서 이쪽으로 올 수는 없는데 일단 만들기*/
		_sector[0][x].downLeftSectorNum = 4;
		_sector[0][x].downLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x].downLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		_sector[0][x].downLeft[2] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x].downLeft[3] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
	}

	// 맨왼쪽 x = 0 설정
	for (int y = 1; y < _sectorYLen - 1; y++)
	{
		// 주변 설정
		_sector[y][0].aroundSectorNum = 6;
		_sector[y][0]._around[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0]._around[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0]._around[2] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0]._around[3] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0]._around[4] = &_sector[y + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		_sector[y][0]._around[5] = &_sector[y][0]; // 본인
		// 왼쪽 이동 설정
		_sector[y][0].leftSectorNum = 0;
		// 오른쪽 이동 설정
		// 오른쪽으로 와서 일로올순 없음
		_sector[y][0].rightSectorNum = 3;
		_sector[y][0].right[0] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0].right[1] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0].right[2] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// 위쪽 이동 설정
		_sector[y][0].upSectorNum = 2;
		_sector[y][0].up[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0].up[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// 아래 이동 설정
		/* 아래로 와서 이쪽으로 올 수는 없는데 일단 만들기*/
		_sector[y][0].downSectorNum = 2;
		_sector[y][0].down[0] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0].down[1] = &_sector[y + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		// 위 오른족 이동 설정
		// 오른쪽으로 와서 일로올순 없음
		_sector[y][0].upRightSectorNum = 4;
		_sector[y][0].upRight[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0].upRight[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0].upRight[2] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0].upRight[3] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// 위 왼쪽 이동 설정
		_sector[y][0].upLeftSectorNum = 2;
		_sector[y][0].upLeft[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0].upLeft[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// 아래 오른쪽 이동 설정
		/* 아래로 와서 이쪽으로 올 수는 없는데 일단 만들기*/
		_sector[y][0].downRightSectorNum = 4;
		_sector[y][0].downRight[0] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0].downRight[1] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0].downRight[2] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0].downRight[3] = &_sector[y + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		//아래 왼쪽 이동 설정
		/* 아래로 와서 이쪽으로 올 수는 없는데 일단 만들기*/
		_sector[y][0].downLeftSectorNum = 2;
		_sector[y][0].downLeft[0] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0].downLeft[1] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
	}

	// 맨 아래 y = _sectorYLen - 1 설정
	for (int x = 1; x < _sectorXLen - 1; x++)
	{
		// 주변 설정
		_sector[_sectorYLen - 1][x].aroundSectorNum = 6;
		_sector[_sectorYLen - 1][x]._around[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][x]._around[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x]._around[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x]._around[3] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x]._around[4] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[_sectorYLen - 1][x]._around[5] = &_sector[_sectorYLen - 1][x]; //자기자신

		// 왼쪽 이동 설정
		_sector[_sectorYLen - 1][x].leftSectorNum = 2;
		_sector[_sectorYLen - 1][x].left[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][x].left[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];

		// 오른쪽 이동 설정
		_sector[_sectorYLen - 1][x].rightSectorNum = 2;
		_sector[_sectorYLen - 1][x].right[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[_sectorYLen - 1][x].right[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		// 위쪽 이동 설정
		// 위로 와서 일로 올순 없음
		_sector[_sectorYLen - 1][x].upSectorNum = 3;
		_sector[_sectorYLen - 1][x].up[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x].up[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x].up[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		// 아래 이동 설정
		_sector[_sectorYLen - 1][x].downSectorNum = 0;
		// 위 오른족 이동 설정
		// 위로 와서 일로 올순 없음
		_sector[_sectorYLen - 1][x].upRightSectorNum = 4;
		_sector[_sectorYLen - 1][x].upRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x].upRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x].upRight[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x].upRight[3] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		// 위 왼쪽 이동 설정
		// 위로 와서 일로 올순 없음
		_sector[_sectorYLen - 1][x].upLeftSectorNum = 4;
		_sector[_sectorYLen - 1][x].upLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x].upLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x].upLeft[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x].upLeft[3] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		// 아래 오른쪽 이동 설정
		_sector[_sectorYLen - 1][x].downRightSectorNum = 2;
		_sector[_sectorYLen - 1][x].downRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x].downRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		//아래 왼쪽 이동 설정
		_sector[_sectorYLen - 1][x].downLeftSectorNum = 2;
		_sector[_sectorYLen - 1][x].downLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][x].downLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LD]];
	}

	// 맨 오른쪽 x = _sectorXLen - 1 섲렁
	for (int y = 1; y < _sectorYLen - 1; y++)
	{
		// 주변 설정
		_sector[y][_sectorXLen - 1].aroundSectorNum = 6;
		_sector[y][_sectorXLen - 1]._around[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1]._around[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1]._around[2] = &_sector[y + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1]._around[3] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[y][_sectorXLen - 1]._around[4] = &_sector[y + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[y][_sectorXLen - 1]._around[5] = &_sector[y][_sectorXLen - 1]; // 본인
		// 왼쪽 이동 설정
		// 왼쪽으로 와서 일로올순없음
		_sector[y][_sectorXLen - 1].leftSectorNum = 3;
		_sector[y][_sectorXLen - 1].left[0] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1].left[1] = &_sector[y + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1].left[2] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// 오른쪽 이동 설정
		_sector[y][0].rightSectorNum = 0;
		// 위쪽 이동 설정
		_sector[y][_sectorXLen - 1].upSectorNum = 2;
		_sector[y][_sectorXLen - 1].up[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1].up[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// 아래 이동 설정
		_sector[y][_sectorXLen - 1].downSectorNum = 2;
		_sector[y][_sectorXLen - 1].down[0] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[y][_sectorXLen - 1].down[1] = &_sector[y + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		// 위 오른족 이동 설정
		_sector[y][_sectorXLen - 1].upRightSectorNum = 2;
		_sector[y][_sectorXLen - 1].upRight[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1].upRight[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// 위 왼쪽 이동 설정
		// 왼쪽으로 와서 일로 올순 없음
		_sector[y][_sectorXLen - 1].upLeftSectorNum = 4;
		_sector[y][_sectorXLen - 1].upLeft[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1].upLeft[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1].upLeft[2] = &_sector[y + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1].upLeft[3] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// 아래 오른쪽 이동 설정
		/* 아래로 와서 이쪽으로 올 수는 없는데 일단 만들기*/
		_sector[y][_sectorXLen - 1].downRightSectorNum = 2;
		_sector[y][_sectorXLen - 1].downRight[0] = &_sector[y + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[y][_sectorXLen - 1].downRight[1] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		//아래 왼쪽 이동 설정
		// 왼쪽으로 와서 일로올순없ㅇ므
		_sector[y][_sectorXLen - 1].downLeftSectorNum = 4;
		_sector[y][_sectorXLen - 1].downLeft[0] = &_sector[0 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1].downLeft[1] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1].downLeft[2] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[y][_sectorXLen - 1].downLeft[3] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
	}

	// 0, 0 설정
	{
		// 주변 설정
		_sector[0][0].aroundSectorNum = 4;
		_sector[0][0]._around[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0]._around[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0]._around[2] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		_sector[0][0]._around[3] = &_sector[0][0];
		// 왼쪽 이동 설정
		_sector[0][0].leftSectorNum = 0;
		// 오른쪽 이동 설정
		_sector[0][0].rightSectorNum = 2;
		_sector[0][0].right[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0].right[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// 위쪽
		_sector[0][0].upSectorNum = 0;
		// 아래
		_sector[0][0].downSectorNum = 2;
		_sector[0][0].down[0] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0].down[1] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		// 위 오른쪽
		_sector[0][0].upRightSectorNum = 2;
		_sector[0][0].upRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0].upRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// 위 왼쪽
		_sector[0][0].upLeftSectorNum = 0;
		// 아래 오른쪽
		_sector[0][0].downRightSectorNum = 3;
		_sector[0][0].downRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0].downRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0].downRight[2] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		//아래 왼쪽
		_sector[0][0].downLeftSectorNum = 2;
		_sector[0][0].downLeft[0] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0].downLeft[1] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
	}
	// 0, _sectorXLen-1 설정
	{
		// 주변 설정
		_sector[0][_sectorXLen - 1].aroundSectorNum = 4;
		_sector[0][_sectorXLen - 1]._around[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1]._around[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[0][_sectorXLen - 1]._around[2] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[0][_sectorXLen - 1]._around[3] = &_sector[0][_sectorXLen - 1];
		// 왼쪽 이동 설정
		_sector[0][_sectorXLen - 1].leftSectorNum = 2;
		_sector[0][_sectorXLen - 1].right[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1].right[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// 오른쪽 이동 설정
		_sector[0][_sectorXLen - 1].rightSectorNum = 0;
		// 위쪽
		_sector[0][_sectorXLen - 1].upSectorNum = 0;
		// 아래
		_sector[0][_sectorXLen - 1].downSectorNum = 2;
		_sector[0][_sectorXLen - 1].down[0] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[0][_sectorXLen - 1].down[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// 위 오른쪽
		_sector[0][_sectorXLen - 1].upRightSectorNum = 0;
		// 위 왼쪽
		_sector[0][_sectorXLen - 1].upLeftSectorNum = 2;
		_sector[0][_sectorXLen - 1].upLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1].upLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// 아래 오른쪽
		_sector[0][_sectorXLen - 1].downRightSectorNum = 2;
		_sector[0][_sectorXLen - 1].downRight[0] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[0][_sectorXLen - 1].downRight[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		//아래 왼쪽
		_sector[0][_sectorXLen - 1].downLeftSectorNum = 3;
		_sector[0][_sectorXLen - 1].downLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1].downLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[0][_sectorXLen - 1].downLeft[2] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
	}


	// _sectorYLen-1, 0 설정
	{
		// 주변 설정
		_sector[_sectorYLen - 1][0].aroundSectorNum = 4;
		_sector[_sectorYLen - 1][0]._around[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0]._around[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0]._around[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[_sectorYLen - 1][0]._around[3] = &_sector[_sectorYLen - 1][0];
		// 왼쪽 이동 설정
		_sector[_sectorYLen - 1][0].leftSectorNum = 0;
		// 오른쪽 이동 설정
		_sector[_sectorYLen - 1][0].rightSectorNum = 2;
		_sector[_sectorYLen - 1][0].right[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0].right[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		// 위쪽
		_sector[_sectorYLen - 1][0].upSectorNum = 2;
		_sector[_sectorYLen - 1][0].up[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0].up[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// 아래
		_sector[_sectorYLen - 1][0].downSectorNum = 0;
		// 위 오른쪽
		_sector[_sectorYLen - 1][0].upRightSectorNum = 3;
		_sector[_sectorYLen - 1][0].upRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0].upRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0].upRight[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		// 위 왼쪽
		_sector[_sectorYLen - 1][0].upLeftSectorNum = 2;
		_sector[_sectorYLen - 1][0].upLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0].upLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// 아래 오른쪽
		_sector[_sectorYLen - 1][0].downRightSectorNum = 2;
		_sector[_sectorYLen - 1][0].downRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0].downRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		//아래 왼쪽
		_sector[_sectorYLen - 1][0].downLeftSectorNum = 0;
	}

	// _sectorYLen-1, _sectorXLen-1설정
	{
		// 주변 설정
		_sector[_sectorYLen - 1][_sectorXLen - 1].aroundSectorNum = 4;
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[3] = &_sector[_sectorYLen - 1][_sectorXLen - 1];
		// 왼쪽 이동 설정
		_sector[_sectorYLen - 1][_sectorXLen - 1].leftSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].left[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].left[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		// 오른쪽 이동 설정
		_sector[_sectorYLen - 1][_sectorXLen - 1].rightSectorNum = 0;
		// 위쪽
		_sector[_sectorYLen - 1][_sectorXLen - 1].upSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].up[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].up[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// 아래
		_sector[_sectorYLen - 1][_sectorXLen - 1].downSectorNum = 0;
		// 위 오른쪽
		_sector[_sectorYLen - 1][_sectorXLen - 1].upRightSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].upRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].upRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// 위 왼쪽
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeftSectorNum = 3;
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeft[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];

		// 아래 오른쪽
		_sector[_sectorYLen - 1][_sectorXLen - 1].downRightSectorNum = 0;

		//아래 왼쪽
		_sector[_sectorYLen - 1][_sectorXLen - 1].downLeftSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].downLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].downLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
	}


}





