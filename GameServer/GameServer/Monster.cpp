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
	// monsterId : 1�� ����
	static int64 monsterIdGenerator = 0;
	_monsterInfo.MonsterID = ++monsterIdGenerator;
	_monsterInfo.Type = type;
	_gameGameThread = gameGameThread;
	_position = position;
}

void Monster::Update(float deltaTime)
{
	//���⼭ �ϴ°� ���� �����ΰ�
	// ���� ��Ÿ�� ���
	_attackTimer += deltaTime;

	switch(_state)
	{
		case MonsterState::MS_IDLE:
		{
			// idle ���¿�����
			_idleTime -= deltaTime;
			if(_idleTime <= 0)
			{
				// idle �ð��� �� �Ǿ�����
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
		// �������� ����������
		_state = MonsterState::MS_IDLE;
		_idleTime = 5.0f;
	}
	else
	{
		// �������� �̵�

		// �ӵ� * deltaTime
		float moveDist = _speed * deltaTime;
		if (moveDist > distance)
		{
			moveDist = distance;
		}


		// �̵��Ÿ� * x���� / �Ÿ�
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

				//���� ����
				// ĳ���� �����ϰ�
				// Ŭ���̾�Ʈ�� ���� ���� ��Ŷ ��
				// ������ ��Ŷ ����
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
			// maxChaseTime �ʰ��ϸ�
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
	//TODO: Ŭ���̾�Ʈ�� ���� �̵� ��Ŷ ����
	CPacket* packet = CPacket::Alloc();
	MP_SC_SPAWN_MONSTER(packet, _monsterInfo, _position);
	_gameGameThread->SendPacket_BroadCast(packet);
	CPacket::Free(packet);
}

void Monster::SetRandomDestination()
{
	//TODO: ���� ���� ��ġ �������
	// ���� ������ ���Ѵ�
	float range = 500.0f; // ���� ����
	_destination.X = _position.X + (rand() % static_cast<int>(range * 2)) - range;
	_destination.Y = _position.Y + (rand() % static_cast<int>(range * 2)) - range;

	_destination.X = std::clamp(_destination.X, MIN_MAP_SIZE_X, MAX_MAP_SIZE_X);
	_destination.Y = std::clamp(_destination.Y, MIN_MAP_SIZE_Y, MAX_MAP_SIZE_Y);

	//���⼭���� �̵��ҰŴϱ� �̵���Ŷ ������ �ǳ�
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

	//�ϴ� ��Ŷ �������ϰ� // �ϴ� ��Ŷ�� ������ ? �� ��Ŷ�� ������

}

float Monster::GetDistanceToPlayer(Player* player)
{
	float dirX = player->Position.X - _position.X;
	float dirY = player->Position.Y - _position.Y;
	return sqrt(dirX * dirX + dirY * dirY);
}

//void Monster::SendIdlePacket()
//{
//	// ���߿� ���� ����� ? ���� ����� �� �������
//		
//
//}
