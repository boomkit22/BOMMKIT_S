#include "Player.h"
#include "Type.h"
#include "GameData.h"
#include "SerializeBuffer.h" 
#include "PacketMaker.h"
#include "Monster.h"

using namespace std;


void Player::Init(int64 sessionId)
{
	_objectId = sessionId;
	_lastRecvTime = 0;
	_bLogined = false;
    playerInfo.Hp = 100;
	Position = { 0, 0,  PLAYER_Z_VALUE };
	playerInfos.clear();

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

void Player::HandleCharacterMove(FVector destination, FRotator startRotation)
{
    CPacket* movePacket = CPacket::Alloc();
    int64 playerId = playerInfo.PlayerID;
    MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, playerId, destination, startRotation);
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
    SendPacket_Around(stopPacket);
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

