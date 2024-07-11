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


// 이거 몬스터 override를 해서 비선공형 선공형을 나눠야할까?
// 아니면 MonsterType을 만들어서 비선공형 선공형을 나눠야할까?
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
	FVector _position; // 현재 위치
	FRotator _rotation;

	FVector _destination; // 목적지
	float _speed; // 이동속도
	MonsterState _state;
	

	Player* _targetPlayer; // 공격중인 플레이어
	float _attackRange; // 공격 범위
	float _attackCooldown; // 공격 쿨타임
	float _attackTimer = 0; // 공격 후 타이머 리셋하기
	float _aggroRange; // 어그로 범위
	float _idleTime;// 일정시간 동안 대기할 시간
	float _chaseTime;// 플레이어를 추적하는 시간
	float _maxChaseTime; // 최대 추적시간

	//
	int _health; // 체력
	int _damage; // 공격력
};

