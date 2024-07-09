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
	//여기서 하는게 좋은 설계인가
	attackTimer += deltaTime;

	switch(state)
	{
		case MonsterState::MS_IDLE:
		{
			// idle 상태였으면
			idleTime -= deltaTime;
			if(idleTime <= 0)
			{
				// idle 시간이 다 되었으면
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
		// 목적지에 도착했으면
		state = MonsterState::MS_IDLE;
		idleTime = 5.0f;
	}
	else
	{
		// 목적지로 이동

		// 속도 * deltaTime
		float moveDist = speed * deltaTime;
		if (moveDist > distance)
		{
			moveDist = distance;
		}


		// 이동거리 * x방향 / 거리
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

				//공격 로직
				// 캐릭터 공경하고
				// 클라이언트에 몬스터 공격 패킷 및
				// 데미지 패킷 전송
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
			// maxChaseTime 초과하면
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
	//TODO: 클라이언트에 몬스터 이동 패킷 전송
}

void Monster::SetRandomDestination()
{
	//TODO: 현재 몬스터 위치 기반으로
	// 랜덤 목적지 정한다
	float range = 100.0f; // 범위 설정
	destination.X = position.X + (rand() % static_cast<int>(range * 2)) - range;
	destination.Y = position.Y + (rand() % static_cast<int>(range * 2)) - range;
	destination.Z = position.Z; // z 좌표는 변경하지 않음
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

	//일단 패킷 보내야하고
}










float Monster::GetDistanceToPlayer(Player* player)
{
	float dirX = player->Position.X - position.X;
	float dirY = player->Position.Y - position.Y;
	return sqrt(dirX * dirX + dirY * dirY);
}