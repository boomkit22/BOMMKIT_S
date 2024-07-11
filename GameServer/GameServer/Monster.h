#pragma once
#include "Type.h"
#include "GameGameThread.h"

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
	friend class GameGameThread;
public:
	void Init(std::unordered_map<int64, Player*>* playerMap,
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


//private:
//	void SendIdlePacket();
//	void SendAttackPacket();
//	void SendDestinationPacket();




	
private:
	std::unordered_map<int64, Player*>* _playerMap;
	MonsterInfo _monsterInfo;
	FVector _position; // ���� ��ġ
	FRotator _rotation;

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

