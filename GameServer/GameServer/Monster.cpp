#include "Monster.h"
#include <cmath>
#include "Player.h"
#include "GameData.h"
#include <algorithm>
#include "GamePacketMaker.h"
#include "SerializeBuffer.h"
#include "GuardianFieldThread.h"

Monster::Monster()
{
}

void Monster::Init(GuardianFieldThread* GuardianFieldThread,
	FVector position, uint16 type)
{
	// monsterId : 1�� ����
	static int64 monsterIdGenerator = 0;
	_monsterInfo.MonsterID = ++monsterIdGenerator;
	_monsterInfo.Type = type;
	_GuardianFieldThread = GuardianFieldThread;
	_position = position;
	_state = MonsterState::MS_IDLE;
	_speed = 200.0f;
	_destination.Z = 88.1;
	_idleTime = _defaultIdleTime;
	_attackRange = 200.0f;
	_attackCooldown = 5.0f;
	_chaseTime = 0;
	_maxChaseTime = 5.0f;
	_aggroRange = 1000.0f;
	_health = 100;
	_damage = 5;
	_rotation = { 0,0,0 };
	_targetPlayer = nullptr;
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

		case MonsterState::MS_DEATH:
		{
			//TODO: �� �ƹ��͵� �Ұ� ����
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

	if (distance < 10.0f)
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

// ĳ���Ͱ� ���������ų� �ϸ� �� targetPlayer�� ����ִ� �ִ� ����ؾ��ϴ°���
void Monster::AttackPlayer(float deltaTime)
{
	if (_targetPlayer)
	{
		float distance = GetDistanceToPlayer(_targetPlayer);
		if (distance <= _attackRange)
		{
			//���ݹ����ȿ���������
			// ���� ����
			//���� �ð� �ʱ�ȭ
			_chaseTime = 0.f;

			if (_attackTimer >= _attackCooldown)
			{
				CalculateRotation(_position, _targetPlayer->Position);
				//�ϴ� Stop�� �ؾ��ϳ�
				
				//Stop Packet�� �׳� �����ô�
	/*			CPacket*  monsterStopPacket = CPacket::Alloc();
				MP_SC_GAME_RES_MONSTER_STOP(monsterStopPacket, _monsterInfo.MonsterID, _position, _rotation);
				_GuardianFieldThread->SendPacket_BroadCast(monsterStopPacket);
				CPacket::Free(monsterStopPacket);*/

				//���⼭ �����ϸ� �ȴ�
				//Ŭ���̾�Ʈ�� ���� ���� �ִϸ��̼� ���� ��
				//�ٵ� �����̼��� ĳ���͸� ���ؼ� �����;��ϳ�
				CPacket* monsterSkilPacket = CPacket::Alloc();
				int32 SkillID = 1;
				MP_SC_GAME_RES_MONSTER_SKILL(monsterSkilPacket, _monsterInfo.MonsterID, _position, _rotation, SkillID);
				_GuardianFieldThread->SendPacket_BroadCast(monsterSkilPacket);
				CPacket::Free(monsterSkilPacket);
				
				//������ ��Ŷ ����
				//���������� ������ ��Ŷ ��������
				CPacket* resDamagePacket = CPacket::Alloc();
				int32 AttackerType = TYPE_MONSTER;
				int64 AttackerID = _monsterInfo.MonsterID;
				int32 targetType = TYPE_PLAYER;
				int64 TargetID = _targetPlayer->playerInfo.PlayerID;
				int32 Damage = _damage;
				MP_SC_GAME_RES_DAMAGE(resDamagePacket, AttackerType, AttackerID, targetType, TargetID, Damage);
				_GuardianFieldThread->SendPacket_BroadCast(resDamagePacket);
				CPacket::Free(resDamagePacket);
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
			//idle���¶�� ������ MonsterStop ��Ŷ
			/*CPacket* idlePacket = CPacket::Alloc();
			MP_SC_GAME_RES_MONSTER_STOP(idlePacket, _monsterInfo.MonsterID, _position, _rotation);
			_GuardianFieldThread->SendPacket_BroadCast(idlePacket);
			CPacket::Free(idlePacket)*/;

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
			//�ÿ��̾� ���� ���� �Ÿ�
			float dirX = _targetPlayer->Position.X - _position.X;
			float dirY = _targetPlayer->Position.Y - _position.Y;
			float length = sqrt(dirX * dirX + dirY * dirY);

			//����ȭ �ϰ�
			dirX /= length;
			dirY /= length;

			// attackRagne���ؼ�
			FVector Destination{
				_targetPlayer->Position.X - dirX * (_attackRange - 10), // ���� �ȿ���
				_targetPlayer->Position.Y - dirY * (_attackRange - 10),
				_position.Z
			};

			SetDestination(Destination);
			MoveToDestination(deltaTime);
		}
	}
}


void Monster::SetDestination(FVector dest)
{
	_destination = dest;
	//TODO: Ŭ���̾�Ʈ�� ���� �̵� ��Ŷ ����
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, dest, _rotation);
	_GuardianFieldThread->SendPacket_BroadCast(packet);
	printf("send monster move location : mosterID : %lld\n", _monsterInfo.MonsterID);


	CPacket::Free(packet);
	CalculateRotation(_position, _destination);
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
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, _destination, _rotation);
	_GuardianFieldThread->SendPacket_BroadCast(packet);

	printf("send monster move location : mosterID : %lld\n", _monsterInfo.MonsterID);

	CPacket::Free(packet);
	CalculateRotation(_position, _destination);
}

void Monster::TakeDamage(int damage, Player* attacker)
{
	if (_targetPlayer!= attacker)
	{
		// �ٸ���
		//�Ұ� ��������
		_chaseTime = 0.0f;
	}

	_health -= damage;
	if (_health > 0)
	{
		_targetPlayer = attacker;
		_state = MonsterState::MS_ATTACKING;
	}
	else {
		//���� �׾����� ��Ŷ ������
		CPacket* diePacket = CPacket::Alloc();
		MP_SC_GAME_RES_MONSTER_DEATH(diePacket, _monsterInfo.MonsterID, _position, _rotation);
		_GuardianFieldThread->SendPacket_BroadCast(diePacket);
		CPacket::Free(diePacket);

		// �� ���ؾ�����
		// ���� Ǯ�� �־���ϳ�?
		_state = MonsterState::MS_DEATH;
	}

	//�ϴ� ��Ŷ �������ϰ� // �ϴ� ��Ŷ�� ������ ? �� ��Ŷ�� ������

}

float Monster::GetDistanceToPlayer(Player* player)
{
	float dirX = player->Position.X - _position.X;
	float dirY = player->Position.Y - _position.Y;
	return sqrt(dirX * dirX + dirY * dirY);
}

FRotator Monster::CalculateRotation(const FVector& oldPosition, const FVector& newPosition)
{
	double dx = newPosition.X - oldPosition.X;
	double dy = newPosition.Y - oldPosition.Y;
	if (dx != 0 || dy != 0) { // �������� ���� ��쿡�� ȸ�� ���
		_rotation.Yaw = std::atan2(dy, dx) * 180 / M_PI; // ���ȿ��� ��(degree)�� ��ȯ
		//std::cout << "New rotation: " << rotation << " degrees" << std::endl; // ����� ���
	}

	return FRotator{0, _rotation.Yaw, 0 };
}

void Monster::SetTargetPlayerEmpty()
{
	_targetPlayer = nullptr;
	_state = MonsterState::MS_IDLE;
	_idleTime = _defaultIdleTime;

	//idle���¶�� ������ MonsterStop ��Ŷ
	CPacket* idlePacket = CPacket::Alloc();
	MP_SC_GAME_RES_MONSTER_STOP(idlePacket, _monsterInfo.MonsterID, _position, _rotation);
	_GuardianFieldThread->SendPacket_BroadCast(idlePacket);
	CPacket::Free(idlePacket);
}
