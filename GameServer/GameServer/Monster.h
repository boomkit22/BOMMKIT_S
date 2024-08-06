#pragma once
#include "Type.h"
#include <unordered_map>
#include "FieldObject.h"
#include "JumpPointSearch.h"
#include <vector>

class Sector;
class CPacket;
class Player;
class BasePacketHandleThread;

//200 �̻��̸� ��ã�� ��û
//200 ���ϸ� ��ã�� ��û����

#define REQUEST_FIND_PATH_THRESHOLD 200.0

enum class MonsterState {
	MS_IDLE,
	MS_MOVING,
	MS_ATTACKING,
	MS_CHASING,
	MS_DEATH,
};


// �̰� ���� override�� �ؼ� �񼱰��� �������� �������ұ�?
// �ƴϸ� MonsterType�� ���� �񼱰��� �������� �������ұ�?
class Monster : public FieldObject
{
	friend class GuardianFieldThread;
	friend class LobbyFieldThread;
	friend class SpiderFieldThread;
	friend class FieldPacketHandleThread;

public:
	//Getter
	MonsterInfo GetMonsterInfo() { return _monsterInfo; };
	FVector GetPosition() { return _position; };
	FRotator GetRotation() { return _rotation; };
	FVector GetDestination() { return _destination; };

public:
	Monster(FieldPacketHandleThread* field, uint16 objectType, uint16 monsterType);
	Monster(FieldPacketHandleThread* field, uint16 objectType, uint16 monsterType, FVector spawnPosition);
	MonsterState GetState() { return _state; };
	void Update(float deltaTime);
	void SetDestination(FVector destination);
	//void HandleAsyncFindPath();
	/// <summary>
	/// 
	/// </summary>
	/// <param name="damage"></param>
	/// <param name="attacker"></param>
	/// <returns>return true when death</returns>
	bool TakeDamage(int damage, Player* attacker);
	void MoveToDestination(float deltaTime);
	void AttackPlayer(float deltaTime);
	void ChasePlayer(float deltaTime);
	void SetRandomDestination();
	float GetDistanceToPlayer(Player* targetPlayer);
	void OnSpawn();

//private:
//	void SendIdlePacket();
//	void SendAttackPacket();
//	void SendDestinationPacket();
	
	//Player�� �����ų� ������ �ش� player�� target���� �ϰ��ִ��ָ� idle ���·� ��������
	void SetTargetPlayerEmpty();

private:
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
	int _damage = 5;	
	float _defaultIdleTime = 10;

private:
	void ProcessSectorChange(Sector* newSector);
	void AddSector(Sector* newSEctor);
	void RemoveSector(Sector* newSector);
//
//private:
//	std::vector<Pos> _path;
//	uint16 _pathIndex = 0;
//	bool bRequestPath = false;
};

