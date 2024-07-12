#pragma once
#include "Type.h"
#include <unordered_map>

#define M_PI 3.141592653589
class Player;
class GameGameThread;
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
	friend class GameGameThread;
public:
	Monster();

	void Init(GameGameThread* gameGameThread,
	FVector position,
	uint16 type);
 
	void Update(float deltaTime);
	void SetDestination(FVector destination);
	void TakeDamage(int damage, Player* attacker);
	void MoveToDestination(float deltaTime);
	void AttackPlayer(float deltaTime);
	void ChasePlayer(float deltaTime);
	void SetRandomDestination();
	float GetDistanceToPlayer(Player* targetPlayer);
	void CalculateRotation(const FVector& oldPosition, const FVector& newPosition);
//private:
//	void SendIdlePacket();
//	void SendAttackPacket();
//	void SendDestinationPacket();




	
private:
	GameGameThread* _gameGameThread;
	MonsterInfo _monsterInfo;
	FVector _position; // ���� ��ġ
	FRotator _rotation{ 0,0,0 }; // ���� ����

	FVector _destination; // ������
	float _speed; // �̵��ӵ�
	MonsterState _state;
	

	Player* _targetPlayer; // �������� �÷��̾�
	float _attackRange; // ���� ����
	float _attackCooldown; // ���� ��Ÿ��
	float _attackTimer = 0; // ���� �� Ÿ�̸� �����ϱ�
	float _aggroRange; // ��׷� ����
	float _idleTime;// �����ð� ���� ����� �ð�
	float _chaseTime;// �÷��̾ �����ϴ� �ð�
	float _maxChaseTime; // �ִ� �����ð�

	//
	int _health; // ü��
	int _damage; // ���ݷ�
};

