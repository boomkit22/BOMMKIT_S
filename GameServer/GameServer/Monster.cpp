#include "Monster.h"
#include <cmath>
#include "Player.h"
#include "GameData.h"
#include <algorithm>
#include "GamePacketMaker.h"
#include "SerializeBuffer.h"
#include "GameGameThread.h"

Monster::Monster()
{
}

void Monster::Init(GameGameThread* gameGameThread,
	FVector position, uint16 type)
{
	// monsterId : 1씩 증가
	static int64 monsterIdGenerator = 0;
	_monsterInfo.MonsterID = ++monsterIdGenerator;
	_monsterInfo.Type = type;
	_gameGameThread = gameGameThread;
	_position = position;
	_state = MonsterState::MS_IDLE;
	_speed = 300.0f;
	_destination.Z = 88.1;
}

void Monster::Update(float deltaTime)
{
	//여기서 하는게 좋은 설계인가
	// 공격 쿨타임 계산
	_attackTimer += deltaTime;

	switch(_state)
	{
		case MonsterState::MS_IDLE:
		{
			// idle 상태였으면
			_idleTime -= deltaTime;
			if(_idleTime <= 0)
			{
				// idle 시간이 다 되었으면
				SetRandomDestination();
				_state = MonsterState::MS_MOVING;
			}
		}
		break;

		case MonsterState::MS_MOVING:
		{
			MoveToDestination(deltaTime);
		}
		break;

		case MonsterState::MS_ATTACKING:
		{
			AttackPlayer(deltaTime);
		}
		break;

		case MonsterState::MS_CHASING:
		{
			ChasePlayer(deltaTime);
		}
		break;

		default:
			__debugbreak();
	}
}

void Monster::MoveToDestination(float deltaTime)
{
	float dirX = _destination.X - _position.X;
	float dirY = _destination.Y - _position.Y;
	float distance = sqrt(dirX * dirX + dirY * dirY);

	if (distance < 1.0f)
	{
		// 목적지에 도착했으면
		_state = MonsterState::MS_IDLE;
		_idleTime = 5.0f;
	}
	else
	{
		// 목적지로 이동

		// 속도 * deltaTime
		float moveDist = _speed * deltaTime * 1000;
		if (moveDist > distance)
		{
			moveDist = distance;
		}


		// 이동거리 * x방향 / 거리
		float moveX = moveDist * dirX / distance;
		float moveY = moveDist * dirY / distance;

		_position.X += moveX;
		_position.Y += moveY;
	}
}

void Monster::AttackPlayer(float deltaTime)
{
	if (_targetPlayer)
	{
		float distance = GetDistanceToPlayer(_targetPlayer);
		if (distance <= _attackRange)
		{
			if (_attackTimer >= _attackCooldown)
			{

				//공격 로직
				// 캐릭터 공경하고
				// 클라이언트에 몬스터 공격 패킷 및
				// 데미지 패킷 전송
				_attackTimer = 0;
			}
		}
		else {
			_state = MonsterState::MS_CHASING;
		}

	}
}

void Monster::ChasePlayer(float deltaTime)
{
	if (_targetPlayer)
	{
		_chaseTime += deltaTime;

		if (_chaseTime > _maxChaseTime)
		{
			// maxChaseTime 초과하면
			_state = MonsterState::MS_IDLE;
			_idleTime = 5.0f;
			_targetPlayer = nullptr;
			_chaseTime = 0;
			return;
		}

		float distance = GetDistanceToPlayer(_targetPlayer);
		if (distance <= _attackRange)
		{
			_state = MonsterState::MS_ATTACKING;
		}
		else {
			SetDestination(_targetPlayer->Position);
			MoveToDestination(deltaTime);
		}
	}
}


void Monster::SetDestination(FVector dest)
{
	_destination = dest;
	//TODO: 클라이언트에 몬스터 이동 패킷 전송
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, dest, _rotation);
	_gameGameThread->SendPacket_BroadCast(packet);
	printf("send monster move location : mosterID : %lld\n", _monsterInfo.MonsterID);


	CPacket::Free(packet);
	CalculateRotation(_position, _destination);
}

void Monster::SetRandomDestination()
{
	//TODO: 현재 몬스터 위치 기반으로
	// 랜덤 목적지 정한다
	float range = 500.0f; // 범위 설정
	_destination.X = _position.X + (rand() % static_cast<int>(range * 2)) - range;
	_destination.Y = _position.Y + (rand() % static_cast<int>(range * 2)) - range;

	_destination.X = std::clamp(_destination.X, MIN_MAP_SIZE_X, MAX_MAP_SIZE_X);
	_destination.Y = std::clamp(_destination.Y, MIN_MAP_SIZE_Y, MAX_MAP_SIZE_Y);


	//여기서부터 이동할거니까 이동패킷 보내면 되나
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, _destination, _rotation);
	_gameGameThread->SendPacket_BroadCast(packet);

	printf("send monster move location : mosterID : %lld\n", _monsterInfo.MonsterID);

	CPacket::Free(packet);
	CalculateRotation(_position, _destination);
}

void Monster::TakeDamage(int damage, Player* attacker)
{
	_health -= damage;
	if (_health > 0)
	{
		_targetPlayer = attacker;
		_state = MonsterState::MS_ATTACKING;
	}
	else {
		//TODO: die
	}

	//일단 패킷 보내야하고 // 일단 패킷을 보낸다 ? 뭔 패킷을 보내지

}

float Monster::GetDistanceToPlayer(Player* player)
{
	float dirX = player->Position.X - _position.X;
	float dirY = player->Position.Y - _position.Y;
	return sqrt(dirX * dirX + dirY * dirY);
}

void Monster::CalculateRotation(const FVector& oldPosition, const FVector& newPosition)
{
	double dx = newPosition.X - oldPosition.X;
	double dy = newPosition.Y - oldPosition.Y;
	if (dx != 0 || dy != 0) { // 움직임이 있을 경우에만 회전 계산
		_rotation.Yaw = std::atan2(dy, dx) * 180 / M_PI; // 라디안에서 도(degree)로 변환
		//std::cout << "New rotation: " << rotation << " degrees" << std::endl; // 디버그 출력
	}
}

//void Monster::SendIdlePacket()
//{
//	// 나중에 섹터 생기면 ? 섹터 생기면 또 어떻게하지
//		
//
//}
