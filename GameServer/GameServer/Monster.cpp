#include "Monster.h"
#include <cmath>
#include "Player.h"

Monster::Monster(int MonsterId, FVector Position)
{
	monsterId = MonsterId;
	position = Position;
}

void Monster::Update(float deltaTime)
{
	//���⼭ �ϴ°� ���� �����ΰ�
	attackTimer += deltaTime;

	switch(state)
	{
		case MonsterState::MS_IDLE:
		{
			// idle ���¿�����
			idleTime -= deltaTime;
			if(idleTime <= 0)
			{
				// idle �ð��� �� �Ǿ�����
				SetRandomDestination();
				state = MonsterState::MS_MOVING;
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
	float dirX = destination.X - position.X;
	float dirY = destination.Y - position.Y;
	float distance = sqrt(dirX * dirX + dirY * dirY);

	if (distance < 1.0f)
	{
		// �������� ����������
		state = MonsterState::MS_IDLE;
		idleTime = 5.0f;
	}
	else
	{
		// �������� �̵�

		// �ӵ� * deltaTime
		float moveDist = speed * deltaTime;
		if (moveDist > distance)
		{
			moveDist = distance;
		}


		// �̵��Ÿ� * x���� / �Ÿ�
		float moveX = moveDist * dirX / distance;
		float moveY = moveDist * dirY / distance;

		position.X += moveX;
		position.Y += moveY;
	}
}

void Monster::AttackPlayer(float deltaTime)
{
	if (targetPlayer)
	{
		float distance = GetDistanceToPlayer(targetPlayer);
		if (distance <= attackRange)
		{
			if (attackTimer >= attackCooldown)
			{

				//���� ����
				// ĳ���� �����ϰ�
				// Ŭ���̾�Ʈ�� ���� ���� ��Ŷ ��
				// ������ ��Ŷ ����
				attackTimer = 0;
			}
		}
		else {
			state = MonsterState::MS_CHASING;
		}

	}
}

void Monster::ChasePlayer(float deltaTime)
{
	if (targetPlayer)
	{
		chaseTime += deltaTime;

		if (chaseTime > maxChaseTime)
		{
			// maxChaseTime �ʰ��ϸ�
			state = MonsterState::MS_IDLE;
			idleTime = 5.0f;
			targetPlayer = nullptr;
			chaseTime = 0;
			return;
		}

		float distance = GetDistanceToPlayer(targetPlayer);
		if (distance <= attackRange)
		{
			state = MonsterState::MS_ATTACKING;
		}
		else {
			SetDestination(targetPlayer->Position);
			MoveToDestination(deltaTime);
		}
	}
}


void Monster::SetDestination(FVector dest)
{
	destination = dest;
	//TODO: Ŭ���̾�Ʈ�� ���� �̵� ��Ŷ ����
}

void Monster::SetRandomDestination()
{
	//TODO: ���� ���� ��ġ �������
	// ���� ������ ���Ѵ�
	float range = 100.0f; // ���� ����
	destination.X = position.X + (rand() % static_cast<int>(range * 2)) - range;
	destination.Y = position.Y + (rand() % static_cast<int>(range * 2)) - range;
	destination.Z = position.Z; // z ��ǥ�� �������� ����
}

void Monster::TakeDamage(int damage, Player* attacker)
{
	health -= damage;
	if (health > 0)
	{
		targetPlayer = attacker;
		state = MonsterState::MS_ATTACKING;
	}
	else {
		//TODO: die
	}

	//�ϴ� ��Ŷ �������ϰ�
}










float Monster::GetDistanceToPlayer(Player* player)
{
	float dirX = player->Position.X - position.X;
	float dirY = player->Position.Y - position.Y;
	return sqrt(dirX * dirX + dirY * dirY);
}