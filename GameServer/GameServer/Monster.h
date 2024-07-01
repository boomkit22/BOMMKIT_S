#pragma once
#include "Type.h"

class Player;

enum class MonsterState {
	MS_IDLE,
	MS_MOVING,
	MS_ATTACKING,
	MS_CHASING,
};


// 이거 몬스터 override를 해서 비선공형 선공형을 나눠야할까?
// 아니면 MonsterType을 만들어서 비선공형 선공형을 나눠야할까?
class Monster
{
public:
	Monster(int MonsterId, FVector Position);

	void Update(float deltaTime);
	void SetDestination(FVector destination);
	void TakeDamage(int damage, Player* attacker);
	void MoveToDestination(float deltaTime);
	void AttackPlayer(float deltaTime);
	void ChasePlayer(float deltaTime);
	void ReturnToIdle(float deltaTime);
	void SetRandomDestination();
	float GetDistanceToPlayer(Player* targetPlayer);
private:
	int monsterId; // 몬스터 고유 id
	FVector position; // 현재 위치
	FVector destination; // 목적지
	float speed; // 이동속도
	MonsterState state;
	

	Player* targetPlayer; // 공격중인 플레이어
	float attackRange; // 공격 범위
	float attackCooldown; // 공격 쿨타임
	float attackTimer = 0; // 공격 후 타이머 리셋하기
	float aggroRange; // 어그로 범위
	float idleTime;// 일정시간 동안 대기할 시간
	float chaseTime;// 플레이어를 추적하는 시간
	float maxChaseTime; // 최대 추적시간

	//
	int health; // 체력
	int damage; // 공격력
};

