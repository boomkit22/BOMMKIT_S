#include "Player.h"
#include "Type.h"
#include "GameData.h"
#include "SerializeBuffer.h" 
#include "PacketMaker.h"
#include "Monster.h"
#include "Sector.h"
#include "FieldPacketHandleThread.h"
#include <utility>
#include "Util.h"
using namespace std;

Player::Player(FieldPacketHandleThread* field, uint16 objectType, int64 sessionId) :FieldObject(field, objectType)
{
	_sessionId = sessionId;
	_lastRecvTime = 0;
	_bLogined = false;
	playerInfo.Hp = 100;
	Position = { 0, 0,  PLAYER_Z_VALUE };
	playerInfos.clear();
	_path.clear();
}

void Player::Update(float deltaTime)
{
	if (bMoving)
	{
		Move(deltaTime);
	}
}

void Player::SetDestination(const FVector& NewDestination)
{
	_destination = NewDestination;
	bMoving = true;

}

void Player::StopMove()
{
    bMoving = false;
}

void Player::OnLeave()
{
	CPacket* despawnPacket = CPacket::Alloc();
	MP_SC_GAME_DESPAWN_OTHER_CHARACTER(despawnPacket, playerInfo.PlayerID);
	SendPacket_Around(despawnPacket, false);
	CPacket::Free(despawnPacket);
}

void Player::OnFieldChange()
{
	// 이캐릭터 패킷 다른캐릭터들에게 보내고
	CPacket* spawnMycharacterPacket = CPacket::Alloc();
	MP_SC_SPAWN_OTHER_CHARACTER(spawnMycharacterPacket, playerInfo, Position, Rotation);
	SendPacket_Around(spawnMycharacterPacket, false);
	CPacket::Free(spawnMycharacterPacket);

	// 이 캐릭터에게 이미 존재하고 있는 다른 FieldObject패킷 보내고
	Sector* currentSector = _currentSector;
	for (int i = 0; i < currentSector->aroundSectorNum; i++)
	{
		std::vector<FieldObject*>& fieldObjectVector = currentSector->_around[i]->fieldObjectVector;
		for (FieldObject* fieldObject : fieldObjectVector)
		{
			uint16 objectType = fieldObject->GetObjectType();

			if (objectType == TYPE_PLAYER)
			{
				Player* otherPlayer = static_cast<Player*>(fieldObject);
				CPacket* spawnOtherCharacterPacket = CPacket::Alloc();
				FVector OtherSpawnLocation = otherPlayer->Position;
				PlayerInfo otherPlayerInfo = otherPlayer->playerInfo;
				

				//플레이어도 이동중었으면 보내야하는
				//spawnOtherCharacterInfo.NickName = p->NickName;
				MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, otherPlayerInfo, OtherSpawnLocation, otherPlayer->Rotation);
				SendPacket_Unicast(_sessionId, spawnOtherCharacterPacket);
				printf("to me send spawn other character\n");
				CPacket::Free(spawnOtherCharacterPacket);

				if(otherPlayer->bMoving)
				{
					CPacket* movePacket = CPacket::Alloc();
					MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, otherPlayer->playerInfo.PlayerID, otherPlayer->Position, otherPlayer->Rotation);
					SendPacket_Unicast(_sessionId, movePacket);
					CPacket::Free(movePacket);
				}

			}
			else if (objectType == TYPE_MONSTER)
			{
				Monster* monster = static_cast<Monster*>(fieldObject);
				CPacket* spawnMonsterPacket = CPacket::Alloc();
				FVector monsterPosition = monster->GetPosition();
				FRotator monsterRotation = monster->GetRotation();
				MonsterInfo monsterInfo = monster->GetMonsterInfo();
				MP_SC_SPAWN_MONSTER(spawnMonsterPacket, monsterInfo, monsterPosition, monsterRotation);
				SendPacket_Unicast(_sessionId, spawnMonsterPacket);
				CPacket::Free(spawnMonsterPacket);

				if (monster->GetState()  == MonsterState::MS_MOVING)
				{
					FVector destination = monster->GetDestination();
					CPacket* movePacket = CPacket::Alloc();
					MP_SC_MONSTER_MOVE(movePacket, monsterInfo.MonsterID, destination, monsterRotation);
					SendPacket_Unicast(_sessionId, movePacket);
					CPacket::Free(movePacket);
				}
			} 
		}
	}

	_currentSector->fieldObjectVector.push_back(this);
}

void Player::HandleCharacterMove(FVector destination)
{
    CPacket* movePacket = CPacket::Alloc();
    int64 playerId = playerInfo.PlayerID;
	Rotation.Yaw = Util::CalculateRotation(Position, destination);
    MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, playerId, destination, Rotation);
    //브로드케스팅
    SendPacket_Around(movePacket);
    CPacket::Free(movePacket);
    SetDestination(destination);
}

void Player::HandleCharacterSkill(FVector startLocation, FRotator startRotation, int32 skillId)
{
    int64 playerId = playerInfo.PlayerID;
    CPacket* resSkillPacket = CPacket::Alloc();
    MP_SC_GAME_RES_CHARACTER_SKILL(resSkillPacket, playerId, startLocation, startRotation, skillId);

    // 이 플레이어 뺴고 브로드캐스팅
    SendPacket_Around(resSkillPacket, false);
    CPacket::Free(resSkillPacket);
}

void Player::HandleCharacterStop(FVector position, FRotator rotation)
{
    int64 playerId = playerInfo.PlayerID;
	CPacket* stopPacket = CPacket::Alloc();
	MP_SC_GAME_RSE_CHARACTER_STOP(stopPacket, playerId, position, rotation);
	//브로드캐스팅
    SendPacket_Around(stopPacket, false);
	CPacket::Free(stopPacket);
}

void Player::HandleCharacterAttack(int32 attackerType, int64 attackerId, int32 targetType, int64 targetId)
{
	int32 damage = _damage;

	if (targetType == TYPE_PLAYER)
	{
		printf("HandleCharacterAttack to player\n");

		//캐릭터 맵에서 찾아서
		//체력 깎고
		Player* targetPlayer = static_cast<Player*>(FindFieldObject(targetId));

		if (targetPlayer == nullptr)
		{
			printf("targetPlayer is nullptr\n");
			return;
		}
		bool bDeath = targetPlayer->TakeDamage(damage);

		printf("targetPlayer->TakeDamage\n");
		CPacket* resDamagePacket = CPacket::Alloc();
		MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerId, targetType, targetId, damage);
		SendPacket_Around(resDamagePacket);
		CPacket::Free(resDamagePacket);

		if (bDeath)
		{
			//죽었으면 죽은 패킷까지 보내고
			CPacket* characterDeathPacket = CPacket::Alloc();
			MP_SC_GAME_RES_CHARACTER_DEATH(characterDeathPacket, targetId, Position, Rotation);
			SendPacket_Around(characterDeathPacket);
			CPacket::Free(characterDeathPacket);
		}
	}

	//TODO: 몬스터에서 검색
	if (targetType == TYPE_MONSTER)
	{
		Monster* targetMonster  = static_cast<Monster*>(FindFieldObject(targetId));
		if (targetMonster == nullptr)
		{
			printf("targetMonster is nullptr\n");
			return;
		}
		
		if (targetMonster->GetState() == MonsterState::MS_DEATH)
		{
			return;
		}

		CPacket* resDamagePacket = CPacket::Alloc();
		MP_SC_GAME_RES_DAMAGE(resDamagePacket, attackerType, attackerId, targetType, targetId, damage);
		SendPacket_Around(resDamagePacket);
		CPacket::Free(resDamagePacket);


		bool bDeath = targetMonster->TakeDamage(damage, this);
		if (bDeath)
		{
			CPacket* monsterDeathPacket = CPacket::Alloc();
			FVector monsterPosition = targetMonster->GetPosition();
			FRotator monsterRotation = targetMonster->GetRotation();
			MP_SC_GAME_RES_MONSTER_DEATH(monsterDeathPacket, targetId, monsterPosition, monsterRotation);
			SendPacket_Around(monsterDeathPacket);
			CPacket::Free(monsterDeathPacket);
		}
	}
}



void Player::Move(float deltaTime) {
	FVector Direction = { _destination.X - Position.X, _destination.Y - Position.Y, 0 };
	double Distance = std::sqrt(Direction.X * Direction.X + Direction.Y * Direction.Y);
	FVector NormalizedDirection = { Direction.X / Distance, Direction.Y / Distance, 0 };

	if (Distance > 0.0) {
		double DistanceToMove = _speed * deltaTime;
		Position.X += NormalizedDirection.X * DistanceToMove;
		Position.Y += NormalizedDirection.Y * DistanceToMove;
	}

	// 이거 범위 얼만큼해야하지
	if (std::abs(Position.X - _destination.X) < 1.0 &&
		std::abs(Position.Y - _destination.Y) < 1.0) {
		Position = _destination; // 목적지에 도달했다고 간주
		bMoving = false;
	}

	double RotationAngleRadians = std::atan2(NormalizedDirection.Y, NormalizedDirection.X);
	double RotationAngleDegrees = RotationAngleRadians * 180 / PI; // 라디안을 도로 언리얼 degree
	Rotation.Yaw = RotationAngleDegrees;


	uint16 newSectorY = Position.Y / _sectorYSize;
	uint16 newSectorX = Position.X / _sectorXSize;

	if (_currentSector->Y == newSectorY && _currentSector->X == newSectorX)
	{
		return;
	}

	printf("ProcessSectorChange\n");
	Sector* currentSector = _currentSector;
	Sector* newSector = GetField()->GetSector(newSectorY, newSectorX);
	ProcessSectorChange(newSector);
}

bool Player::TakeDamage(int32 damage)
{
    //죽으면 tru
    playerInfo.Hp -= damage;
    if (playerInfo.Hp <= 0)
    {
		playerInfo.Hp = 0;
		return true;
	}

    return false;
}

void Player::ProcessSectorChange(Sector* newSector)
{
	AddSector(newSector);
	RemoveSector(newSector);
	_currentSector = newSector;
}

void Player::AddSector(Sector* newSector)
{
	// 1: new Sector에 있는 섹터에 이 캐릭터 생성, 액션 보내고
	Sector** addSector = nullptr;
	int8 addSectorNum = 0;
	int8 moveDirection = diffToDirection[{newSector->Y - _currentSector->Y, newSector->X - _currentSector->X}];
	// 이동 방향에 따라 이 캐릭터에게 새로 추가된 섹터 계산
	switch (moveDirection)
	{
		case MOVE_DIR_LL:
		{
			addSector = newSector->left;
			addSectorNum = newSector->leftSectorNum;
		}
		break;

		case MOVE_DIR_RR:
		{
			addSector = newSector->right;
			addSectorNum = newSector->rightSectorNum;
		}
		break;

		case MOVE_DIR_UU:
		{
			addSector = newSector->up;
			addSectorNum = newSector->upSectorNum;
		}
		break;

		case MOVE_DIR_DD:
		{
			addSector = newSector->down;
			addSectorNum = newSector->downSectorNum;
		}
		break;

		case MOVE_DIR_RU:
		{
			addSector = newSector->upRight;
			addSectorNum = newSector->upRightSectorNum;
		}
		break;

		case MOVE_DIR_LU:
		{
			addSector = newSector->upLeft;
			addSectorNum = newSector->upLeftSectorNum;
		}
		break;

		case MOVE_DIR_RD:
		{
			addSector = newSector->downRight;
			addSectorNum = newSector->downRightSectorNum;
		}
		break;

		case MOVE_DIR_LD:
		{
			addSector = newSector->downLeft;
			addSectorNum = newSector->downLeftSectorNum;
		}
		break;

		default:
		{
			__debugbreak();
		}
	}

	//새로 추가된 섹터에, 이 캐릭터의 생성, 이동시 이동 패킷 보내기
	CPacket* spawnMycharacterPacket = CPacket::Alloc();
	MP_SC_SPAWN_OTHER_CHARACTER(spawnMycharacterPacket, playerInfo, Position, Rotation);
	for (int i = 0; i < addSectorNum; i++)
	{
		SendPacket_Sector(addSector[i], spawnMycharacterPacket);
	}
	CPacket::Free(spawnMycharacterPacket);
	
	if (bMoving)
	{
		CPacket* movingThisChracterPkt = CPacket::Alloc();
		MP_SC_GAME_RES_CHARACTER_MOVE(movingThisChracterPkt, playerInfo.PlayerID, _destination, Rotation);
		for (int i = 0; i < addSectorNum; i++)
		{
			SendPacket_Sector(addSector[i], movingThisChracterPkt);
		}
		CPacket::Free(movingThisChracterPkt);
	}

	// 추가된 섹터에 있는 캐릭터, 몬스터들의 생성, 이동 패킷 보내기

	for (int i = 0; i < addSectorNum; i++)
	{
		std::vector<FieldObject*>& fieldObjectVector = addSector[i]->fieldObjectVector;
		for (FieldObject* fieldObject : fieldObjectVector)
		{
			uint16 objectType = fieldObject->GetObjectType();

			if (objectType == TYPE_PLAYER)
			{
				Player* otherPlayer = static_cast<Player*>(fieldObject);
				CPacket* spawnOtherCharacterPacket = CPacket::Alloc();
				FVector OtherSpawnLocation = otherPlayer->Position;
				PlayerInfo otherPlayerInfo = otherPlayer->playerInfo;


				//플레이어도 이동중었으면 보내야하는
				//spawnOtherCharacterInfo.NickName = p->NickName;
				MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, otherPlayerInfo, OtherSpawnLocation, otherPlayer->Rotation);
				SendPacket_Unicast(_sessionId, spawnOtherCharacterPacket);
				printf("to me send spawn other character\n");
				CPacket::Free(spawnOtherCharacterPacket);

				if (otherPlayer->bMoving)
				{
					CPacket* movePacket = CPacket::Alloc();
					MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, otherPlayer->playerInfo.PlayerID, otherPlayer->Position, otherPlayer->Rotation);
					SendPacket_Unicast(_sessionId, movePacket);
					CPacket::Free(movePacket);
				}

			}
			else if (objectType == TYPE_MONSTER)
			{
				Monster* monster = static_cast<Monster*>(fieldObject);
				CPacket* spawnMonsterPacket = CPacket::Alloc();
				FVector monsterPosition = monster->GetPosition();
				FRotator monsterRotation = monster->GetRotation();
				MonsterInfo monsterInfo = monster->GetMonsterInfo();
				MP_SC_SPAWN_MONSTER(spawnMonsterPacket, monsterInfo, monsterPosition, monsterRotation);
				SendPacket_Unicast(_sessionId, spawnMonsterPacket);
				CPacket::Free(spawnMonsterPacket);

				if (monster->GetState() == MonsterState::MS_MOVING)
				{
					FVector destination = monster->GetDestination();
					CPacket* movePacket = CPacket::Alloc();
					MP_SC_MONSTER_MOVE(movePacket, monsterInfo.MonsterID, destination, monsterRotation);
					SendPacket_Unicast(_sessionId, movePacket);
					CPacket::Free(movePacket);
				}
			}
		}
	}

	newSector->fieldObjectVector.push_back(this);
}

void Player::RemoveSector(Sector* newSector)
{
	//먼저 섹터에서 이 캐릭터 삭제하고
	Sector* nowSector = _currentSector;
	std::vector<FieldObject*>& fieldObjectVector = nowSector->fieldObjectVector;
	auto it = std::find(fieldObjectVector.begin(), fieldObjectVector.end(), this);
	if (it != fieldObjectVector.end())
	{
		fieldObjectVector.erase(it);
	}
	else {
		__debugbreak();
	}

	//섹터가 변경됨에 따라 사라진 섹터 구하기
	Sector** deleteSector = nullptr;
	int8 deleteSectorNum = 0;
	int8 moveDirection = diffToDirection[{newSector->Y - nowSector->Y, newSector->X - nowSector->X}];
	switch (moveDirection)
	{
		case MOVE_DIR_LL:
		{
			deleteSector = nowSector->right;
			deleteSectorNum = nowSector->rightSectorNum;
		}
		break;

		case MOVE_DIR_RR:
		{
			deleteSector = nowSector->left;
			deleteSectorNum = nowSector->leftSectorNum;
		}
		break;

		case MOVE_DIR_UU:
		{
			deleteSector = nowSector->down;
			deleteSectorNum = nowSector->downSectorNum;
		}
		break;

		case MOVE_DIR_DD:
		{
			deleteSector = nowSector->up;
			deleteSectorNum = nowSector->upSectorNum;
		}
		break;

		case MOVE_DIR_RU:
		{
			deleteSector = nowSector->downLeft;
			deleteSectorNum = nowSector->downLeftSectorNum;
		}
		break;

		case MOVE_DIR_LU:
		{
			deleteSector = nowSector->downRight;
			deleteSectorNum = nowSector->downRightSectorNum;
		}
		break;

		case MOVE_DIR_RD:
		{
			deleteSector = nowSector->upLeft;
			deleteSectorNum = nowSector->upLeftSectorNum;
		}
		break;

		case MOVE_DIR_LD:
		{
			deleteSector = nowSector->upRight;
			deleteSectorNum = nowSector->upRightSectorNum;
		}
		break;

		default:
		{
			__debugbreak();
		}
	}

	//벗어난 섹터에 이 캐릭터 삭제 패킷 보내고
	CPacket* deleteThisCharacterPacket = CPacket::Alloc();
	MP_SC_GAME_DESPAWN_OTHER_CHARACTER(deleteThisCharacterPacket, playerInfo.PlayerID);
	for (int i = 0; i < deleteSectorNum; i++)
	{
		SendPacket_Sector(deleteSector[i], deleteThisCharacterPacket);
	}
	CPacket::Free(deleteThisCharacterPacket);

	
	//벗어난 섹터에 있던 캐릭터, 몬스터들 삭제메시지 이 캐릭터한테 보내고
	for (int i = 0; i < deleteSectorNum; i++)
	{
		std::vector<FieldObject*>& fieldObjectVector = deleteSector[i]->fieldObjectVector;
		for (FieldObject* fieldObject : fieldObjectVector)
		{
			uint16 objectType = fieldObject->GetObjectType();

			if (objectType == TYPE_PLAYER)
			{
				CPacket* deleteOtherPlayerPacket = CPacket::Alloc();
				Player* otherPlayer = static_cast<Player*>(fieldObject);
				int64 otherPlayerId = otherPlayer->playerInfo.PlayerID;
				MP_SC_GAME_DESPAWN_OTHER_CHARACTER(deleteOtherPlayerPacket, otherPlayerId);
				SendPacket_Unicast(_sessionId, deleteOtherPlayerPacket);
				CPacket::Free(deleteOtherPlayerPacket);
			}
			else if (objectType == TYPE_MONSTER)
			{
				CPacket* deleteMonsterPacket = CPacket::Alloc();
				int64 objectId = fieldObject->GetObjectId();
				MP_SC_GAME_DESPAWN_MONSTER(deleteMonsterPacket, objectId);
				SendPacket_Unicast(_sessionId, deleteMonsterPacket);
				CPacket::Free(deleteMonsterPacket);
			}
		}
	}
}


