#pragma once
#include "Type.h"

class Player;

enum class MonsterState {
	MS_IDLE,
	MS_MOVING,
	MS_ATTACKING,
	MS_CHASING,
};


// �̰� ���� override�� �ؼ� �񼱰��� �������� �������ұ�?
// �ƴϸ� MonsterType�� ���� �񼱰��� �������� �������ұ�?
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
	int monsterId; // ���� ���� id
	FVector position; // ���� ��ġ
	FVector destination; // ������
	float speed; // �̵��ӵ�
	MonsterState state;
	

	Player* targetPlayer; // �������� �÷��̾�
	float attackRange; // ���� ����
	float attackCooldown; // ���� ��Ÿ��
	float attackTimer = 0; // ���� �� Ÿ�̸� �����ϱ�
	float aggroRange; // ��׷� ����
	float idleTime;// �����ð� ���� ����� �ð�
	float chaseTime;// �÷��̾ �����ϴ� �ð�
	float maxChaseTime; // �ִ� �����ð�

	//
	int health; // ü��
	int damage; // ���ݷ�
};

