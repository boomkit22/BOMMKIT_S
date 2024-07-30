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
	//��ε� ĳ����
	//TODO: �������� �����ϱ�
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
	FRotator spawnRotation{ 0, 0, 0 };
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
		MP_SC_SPAWN_MONSTER(spawnMonsterPacket, monster->_monsterInfo, monster->_position, monster->_rotation);
		SendPacket_Unicast(p->_sessionId, spawnMonsterPacket);
		//printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);
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
	// �翷 ���� �������� ����κи���
	for (int y = 1; y < _sectorYLen - 1; y++)
	{
		for (int x = 1; x < _sectorXLen - 1; x++)
		{
			// around ����
			_sector[y][x].aroundSectorNum = 9;
			for (int i = 0; i < MOVE_DIR_MAX; i++)
			{
				_sector[y][x]._around[i] = &_sector[y + dy[i]][x + dx[i]];
			}
			_sector[y][x]._around[MOVE_DIR_MAX] = &_sector[y][x];

			// ���� �̵� ����
			_sector[y][x].leftSectorNum = 3;
			_sector[y][x].left[0] = &_sector[y + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
			_sector[y][x].left[1] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].left[2] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];

			// ������ �̵� ����
			_sector[y][x].rightSectorNum = 3;
			_sector[y][x].right[0] = &_sector[y + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
			_sector[y][x].right[1] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].right[2] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];

			// ���� �̵� ����
			_sector[y][x].upSectorNum = 3;
			_sector[y][x].up[0] = &_sector[y + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
			_sector[y][x].up[1] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].up[2] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];

			//�Ʒ� �̵� ����
			_sector[y][x].downSectorNum = 3;
			_sector[y][x].down[0] = &_sector[y + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
			_sector[y][x].down[1] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
			_sector[y][x].down[2] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];

			// �� ������ �̵� ����
			_sector[y][x].upRightSectorNum = 5;
			_sector[y][x].upRight[0] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].upRight[1] = &_sector[y + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
			_sector[y][x].upRight[2] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].upRight[3] = &_sector[y + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
			_sector[y][x].upRight[4] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];

			// �� ���� �̵� ����
			_sector[y][x].upLeftSectorNum = 5;
			_sector[y][x].upLeft[0] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].upLeft[1] = &_sector[y + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
			_sector[y][x].upLeft[2] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].upLeft[3] = &_sector[y + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
			_sector[y][x].upLeft[4] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];

			// �Ʒ� ������ �̵� ����
			_sector[y][x].downRightSectorNum = 5;
			_sector[y][x].downRight[0] = &_sector[y + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
			_sector[y][x].downRight[1] = &_sector[y + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
			_sector[y][x].downRight[2] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
			_sector[y][x].downRight[3] = &_sector[y + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
			_sector[y][x].downRight[4] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];

			// �Ʒ� ���� �̵� ����
			_sector[y][x].downLeftSectorNum = 5;
			_sector[y][x].downLeft[0] = &_sector[y + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
			_sector[y][x].downLeft[1] = &_sector[y + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
			_sector[y][x].downLeft[2] = &_sector[y + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
			_sector[y][x].downLeft[3] = &_sector[y + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
			_sector[y][x].downLeft[4] = &_sector[y + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		}
	}

	// ���� y = 0 ����
	for (int x = 1; x < _sectorXLen - 1; x++)
	{
		// �ֺ� ����
		_sector[0][x].aroundSectorNum = 6;
		_sector[0][x]._around[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x]._around[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		_sector[0][x]._around[2] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x]._around[3] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		_sector[0][x]._around[4] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x]._around[5] = &_sector[0][x]; //�ڱ��ڽ�

		// ���� �̵� ����
		_sector[0][x].leftSectorNum = 2;
		_sector[0][x].left[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x].left[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		// ������ �̵� ����
		_sector[0][x].rightSectorNum = 2;
		_sector[0][x].right[0] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x].right[1] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		// ���� �̵� ����
		_sector[0][x].upSectorNum = 0;
		// �Ʒ� �̵� ����
		/* �Ʒ��� �ͼ� �������� �� ���� ���µ� �ϴ� �����*/
		_sector[0][x].downSectorNum = 3;
		_sector[0][x].down[0] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		_sector[0][x].down[1] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x].down[2] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		// �� ������ �̵� ����
		_sector[0][x].upRightSectorNum = 2;
		_sector[0][x].upRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x].upRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		// �� ���� �̵� ����
		_sector[0][x].upLeftSectorNum = 2;
		_sector[0][x].upLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x].upLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		// �Ʒ� ������ �̵� ����
		/* �Ʒ��� �ͼ� �������� �� ���� ���µ� �ϴ� �����*/
		_sector[0][x].downRightSectorNum = 4;
		_sector[0][x].downRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[0][x].downRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
		_sector[0][x].downRight[2] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x].downRight[3] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		//�Ʒ� ���� �̵� ����
		/* �Ʒ��� �ͼ� �������� �� ���� ���µ� �ϴ� �����*/
		_sector[0][x].downLeftSectorNum = 4;
		_sector[0][x].downLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[0][x].downLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][x + dx[MOVE_DIR_LD]];
		_sector[0][x].downLeft[2] = &_sector[0 + dy[MOVE_DIR_DD]][x + dx[MOVE_DIR_DD]];
		_sector[0][x].downLeft[3] = &_sector[0 + dy[MOVE_DIR_RD]][x + dx[MOVE_DIR_RD]];
	}

	// �ǿ��� x = 0 ����
	for (int y = 1; y < _sectorYLen - 1; y++)
	{
		// �ֺ� ����
		_sector[y][0].aroundSectorNum = 6;
		_sector[y][0]._around[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0]._around[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0]._around[2] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0]._around[3] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0]._around[4] = &_sector[y + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		_sector[y][0]._around[5] = &_sector[y][0]; // ����
		// ���� �̵� ����
		_sector[y][0].leftSectorNum = 0;
		// ������ �̵� ����
		// ���������� �ͼ� �Ϸοü� ����
		_sector[y][0].rightSectorNum = 3;
		_sector[y][0].right[0] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0].right[1] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0].right[2] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// ���� �̵� ����
		_sector[y][0].upSectorNum = 2;
		_sector[y][0].up[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0].up[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// �Ʒ� �̵� ����
		/* �Ʒ��� �ͼ� �������� �� ���� ���µ� �ϴ� �����*/
		_sector[y][0].downSectorNum = 2;
		_sector[y][0].down[0] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0].down[1] = &_sector[y + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		// �� ������ �̵� ����
		// ���������� �ͼ� �Ϸοü� ����
		_sector[y][0].upRightSectorNum = 4;
		_sector[y][0].upRight[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0].upRight[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0].upRight[2] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0].upRight[3] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// �� ���� �̵� ����
		_sector[y][0].upLeftSectorNum = 2;
		_sector[y][0].upLeft[0] = &_sector[y + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[y][0].upLeft[1] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// �Ʒ� ������ �̵� ����
		/* �Ʒ��� �ͼ� �������� �� ���� ���µ� �ϴ� �����*/
		_sector[y][0].downRightSectorNum = 4;
		_sector[y][0].downRight[0] = &_sector[y + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[y][0].downRight[1] = &_sector[y + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[y][0].downRight[2] = &_sector[y + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0].downRight[3] = &_sector[y + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		//�Ʒ� ���� �̵� ����
		/* �Ʒ��� �ͼ� �������� �� ���� ���µ� �ϴ� �����*/
		_sector[y][0].downLeftSectorNum = 2;
		_sector[y][0].downLeft[0] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[y][0].downLeft[1] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
	}

	// �� �Ʒ� y = _sectorYLen - 1 ����
	for (int x = 1; x < _sectorXLen - 1; x++)
	{
		// �ֺ� ����
		_sector[_sectorYLen - 1][x].aroundSectorNum = 6;
		_sector[_sectorYLen - 1][x]._around[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][x]._around[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x]._around[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x]._around[3] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x]._around[4] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[_sectorYLen - 1][x]._around[5] = &_sector[_sectorYLen - 1][x]; //�ڱ��ڽ�

		// ���� �̵� ����
		_sector[_sectorYLen - 1][x].leftSectorNum = 2;
		_sector[_sectorYLen - 1][x].left[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][x].left[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];

		// ������ �̵� ����
		_sector[_sectorYLen - 1][x].rightSectorNum = 2;
		_sector[_sectorYLen - 1][x].right[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		_sector[_sectorYLen - 1][x].right[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		// ���� �̵� ����
		// ���� �ͼ� �Ϸ� �ü� ����
		_sector[_sectorYLen - 1][x].upSectorNum = 3;
		_sector[_sectorYLen - 1][x].up[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x].up[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x].up[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		// �Ʒ� �̵� ����
		_sector[_sectorYLen - 1][x].downSectorNum = 0;
		// �� ������ �̵� ����
		// ���� �ͼ� �Ϸ� �ü� ����
		_sector[_sectorYLen - 1][x].upRightSectorNum = 4;
		_sector[_sectorYLen - 1][x].upRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x].upRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x].upRight[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x].upRight[3] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		// �� ���� �̵� ����
		// ���� �ͼ� �Ϸ� �ü� ����
		_sector[_sectorYLen - 1][x].upLeftSectorNum = 4;
		_sector[_sectorYLen - 1][x].upLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x].upLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][x + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][x].upLeft[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][x].upLeft[3] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		// �Ʒ� ������ �̵� ����
		_sector[_sectorYLen - 1][x].downRightSectorNum = 2;
		_sector[_sectorYLen - 1][x].downRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][x + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][x].downRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][x + dx[MOVE_DIR_RR]];
		//�Ʒ� ���� �̵� ����
		_sector[_sectorYLen - 1][x].downLeftSectorNum = 2;
		_sector[_sectorYLen - 1][x].downLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][x + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][x].downLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][x + dx[MOVE_DIR_LD]];
	}

	// �� ������ x = _sectorXLen - 1 �A��
	for (int y = 1; y < _sectorYLen - 1; y++)
	{
		// �ֺ� ����
		_sector[y][_sectorXLen - 1].aroundSectorNum = 6;
		_sector[y][_sectorXLen - 1]._around[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1]._around[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1]._around[2] = &_sector[y + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1]._around[3] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[y][_sectorXLen - 1]._around[4] = &_sector[y + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[y][_sectorXLen - 1]._around[5] = &_sector[y][_sectorXLen - 1]; // ����
		// ���� �̵� ����
		// �������� �ͼ� �Ϸοü�����
		_sector[y][_sectorXLen - 1].leftSectorNum = 3;
		_sector[y][_sectorXLen - 1].left[0] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1].left[1] = &_sector[y + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1].left[2] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// ������ �̵� ����
		_sector[y][0].rightSectorNum = 0;
		// ���� �̵� ����
		_sector[y][_sectorXLen - 1].upSectorNum = 2;
		_sector[y][_sectorXLen - 1].up[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1].up[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// �Ʒ� �̵� ����
		_sector[y][_sectorXLen - 1].downSectorNum = 2;
		_sector[y][_sectorXLen - 1].down[0] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[y][_sectorXLen - 1].down[1] = &_sector[y + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		// �� ������ �̵� ����
		_sector[y][_sectorXLen - 1].upRightSectorNum = 2;
		_sector[y][_sectorXLen - 1].upRight[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1].upRight[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// �� ���� �̵� ����
		// �������� �ͼ� �Ϸ� �ü� ����
		_sector[y][_sectorXLen - 1].upLeftSectorNum = 4;
		_sector[y][_sectorXLen - 1].upLeft[0] = &_sector[y + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[y][_sectorXLen - 1].upLeft[1] = &_sector[y + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1].upLeft[2] = &_sector[y + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1].upLeft[3] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// �Ʒ� ������ �̵� ����
		/* �Ʒ��� �ͼ� �������� �� ���� ���µ� �ϴ� �����*/
		_sector[y][_sectorXLen - 1].downRightSectorNum = 2;
		_sector[y][_sectorXLen - 1].downRight[0] = &_sector[y + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[y][_sectorXLen - 1].downRight[1] = &_sector[y + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		//�Ʒ� ���� �̵� ����
		// �������� �ͼ� �Ϸοü�������
		_sector[y][_sectorXLen - 1].downLeftSectorNum = 4;
		_sector[y][_sectorXLen - 1].downLeft[0] = &_sector[0 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[y][_sectorXLen - 1].downLeft[1] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[y][_sectorXLen - 1].downLeft[2] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[y][_sectorXLen - 1].downLeft[3] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
	}

	// 0, 0 ����
	{
		// �ֺ� ����
		_sector[0][0].aroundSectorNum = 4;
		_sector[0][0]._around[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0]._around[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0]._around[2] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		_sector[0][0]._around[3] = &_sector[0][0];
		// ���� �̵� ����
		_sector[0][0].leftSectorNum = 0;
		// ������ �̵� ����
		_sector[0][0].rightSectorNum = 2;
		_sector[0][0].right[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0].right[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// ����
		_sector[0][0].upSectorNum = 0;
		// �Ʒ�
		_sector[0][0].downSectorNum = 2;
		_sector[0][0].down[0] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0].down[1] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		// �� ������
		_sector[0][0].upRightSectorNum = 2;
		_sector[0][0].upRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0].upRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		// �� ����
		_sector[0][0].upLeftSectorNum = 0;
		// �Ʒ� ������
		_sector[0][0].downRightSectorNum = 3;
		_sector[0][0].downRight[0] = &_sector[0 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[0][0].downRight[1] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0].downRight[2] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
		//�Ʒ� ����
		_sector[0][0].downLeftSectorNum = 2;
		_sector[0][0].downLeft[0] = &_sector[0 + dy[MOVE_DIR_RD]][0 + dx[MOVE_DIR_RD]];
		_sector[0][0].downLeft[1] = &_sector[0 + dy[MOVE_DIR_DD]][0 + dx[MOVE_DIR_DD]];
	}
	// 0, _sectorXLen-1 ����
	{
		// �ֺ� ����
		_sector[0][_sectorXLen - 1].aroundSectorNum = 4;
		_sector[0][_sectorXLen - 1]._around[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1]._around[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[0][_sectorXLen - 1]._around[2] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[0][_sectorXLen - 1]._around[3] = &_sector[0][_sectorXLen - 1];
		// ���� �̵� ����
		_sector[0][_sectorXLen - 1].leftSectorNum = 2;
		_sector[0][_sectorXLen - 1].right[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1].right[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// ������ �̵� ����
		_sector[0][_sectorXLen - 1].rightSectorNum = 0;
		// ����
		_sector[0][_sectorXLen - 1].upSectorNum = 0;
		// �Ʒ�
		_sector[0][_sectorXLen - 1].downSectorNum = 2;
		_sector[0][_sectorXLen - 1].down[0] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[0][_sectorXLen - 1].down[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// �� ������
		_sector[0][_sectorXLen - 1].upRightSectorNum = 0;
		// �� ����
		_sector[0][_sectorXLen - 1].upLeftSectorNum = 2;
		_sector[0][_sectorXLen - 1].upLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1].upLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		// �Ʒ� ������
		_sector[0][_sectorXLen - 1].downRightSectorNum = 2;
		_sector[0][_sectorXLen - 1].downRight[0] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
		_sector[0][_sectorXLen - 1].downRight[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		//�Ʒ� ����
		_sector[0][_sectorXLen - 1].downLeftSectorNum = 3;
		_sector[0][_sectorXLen - 1].downLeft[0] = &_sector[0 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[0][_sectorXLen - 1].downLeft[1] = &_sector[0 + dy[MOVE_DIR_LD]][_sectorXLen - 1 + dx[MOVE_DIR_LD]];
		_sector[0][_sectorXLen - 1].downLeft[2] = &_sector[0 + dy[MOVE_DIR_DD]][_sectorXLen - 1 + dx[MOVE_DIR_DD]];
	}


	// _sectorYLen-1, 0 ����
	{
		// �ֺ� ����
		_sector[_sectorYLen - 1][0].aroundSectorNum = 4;
		_sector[_sectorYLen - 1][0]._around[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0]._around[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0]._around[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		_sector[_sectorYLen - 1][0]._around[3] = &_sector[_sectorYLen - 1][0];
		// ���� �̵� ����
		_sector[_sectorYLen - 1][0].leftSectorNum = 0;
		// ������ �̵� ����
		_sector[_sectorYLen - 1][0].rightSectorNum = 2;
		_sector[_sectorYLen - 1][0].right[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0].right[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		// ����
		_sector[_sectorYLen - 1][0].upSectorNum = 2;
		_sector[_sectorYLen - 1][0].up[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0].up[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// �Ʒ�
		_sector[_sectorYLen - 1][0].downSectorNum = 0;
		// �� ������
		_sector[_sectorYLen - 1][0].upRightSectorNum = 3;
		_sector[_sectorYLen - 1][0].upRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0].upRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0].upRight[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		// �� ����
		_sector[_sectorYLen - 1][0].upLeftSectorNum = 2;
		_sector[_sectorYLen - 1][0].upLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][0 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][0].upLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		// �Ʒ� ������
		_sector[_sectorYLen - 1][0].downRightSectorNum = 2;
		_sector[_sectorYLen - 1][0].downRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RU]][0 + dx[MOVE_DIR_RU]];
		_sector[_sectorYLen - 1][0].downRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_RR]][0 + dx[MOVE_DIR_RR]];
		//�Ʒ� ����
		_sector[_sectorYLen - 1][0].downLeftSectorNum = 0;
	}

	// _sectorYLen-1, _sectorXLen-1����
	{
		// �ֺ� ����
		_sector[_sectorYLen - 1][_sectorXLen - 1].aroundSectorNum = 4;
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		_sector[_sectorYLen - 1][_sectorXLen - 1]._around[3] = &_sector[_sectorYLen - 1][_sectorXLen - 1];
		// ���� �̵� ����
		_sector[_sectorYLen - 1][_sectorXLen - 1].leftSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].left[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].left[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
		// ������ �̵� ����
		_sector[_sectorYLen - 1][_sectorXLen - 1].rightSectorNum = 0;
		// ����
		_sector[_sectorYLen - 1][_sectorXLen - 1].upSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].up[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].up[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// �Ʒ�
		_sector[_sectorYLen - 1][_sectorXLen - 1].downSectorNum = 0;
		// �� ������
		_sector[_sectorYLen - 1][_sectorXLen - 1].upRightSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].upRight[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].upRight[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		// �� ����
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeftSectorNum = 3;
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_UU]][_sectorXLen - 1 + dx[MOVE_DIR_UU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].upLeft[2] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];

		// �Ʒ� ������
		_sector[_sectorYLen - 1][_sectorXLen - 1].downRightSectorNum = 0;

		//�Ʒ� ����
		_sector[_sectorYLen - 1][_sectorXLen - 1].downLeftSectorNum = 2;
		_sector[_sectorYLen - 1][_sectorXLen - 1].downLeft[0] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LU]][_sectorXLen - 1 + dx[MOVE_DIR_LU]];
		_sector[_sectorYLen - 1][_sectorXLen - 1].downLeft[1] = &_sector[_sectorYLen - 1 + dy[MOVE_DIR_LL]][_sectorXLen - 1 + dx[MOVE_DIR_LL]];
	}


}





