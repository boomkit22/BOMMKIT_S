#include "Monster.h"
#include "Player.h"
#include "GameData.h"
#include <algorithm>
#include "PacketMaker.h"
#include "SerializeBuffer.h"
#include "BasePAcketHandleThread.h"
#include "Sector.h"
#include "FieldPacketHandleThread.h"
#include "Util.h"


Monster::Monster(FieldPacketHandleThread* field, uint16 objectType, uint16 monsterType) : FieldObject(field, objectType)
{
	Init(monsterType);
}

Monster::Monster(FieldPacketHandleThread* field, uint16 objectType, uint16 monsterType, FVector spawnPosition) : FieldObject(field, objectType)
{
	// monsterId : 1�� ����
	_position = spawnPosition;
	Init(monsterType);
}

void Monster::Init(uint16 monsterType)
{
	_monsterInfo.MonsterID = _objectId;
	_monsterInfo.Type = monsterType;
	_monsterInfo.Hp = 100;
	_state = MonsterState::MS_IDLE;
	_speed = 200.0f;
	_destination.Z = 88.1;
	_idleTime = _defaultIdleTime;
	_attackRange = 200.0f;
	_attackCooldown = 5.0f;
	_chaseTime = 0;
	_maxChaseTime = 100.0f;
	_aggroRange = 1000.0f;
	_damage = 5;
	_rotation = { 0,0,0 };
	_targetPlayer = nullptr;
	bErase = false;
}

void  Monster::Update(float deltaTime)
{
	//���⼭ �ϴ°� ���� �����ΰ�
	// ���� ��Ÿ�� ���
	_attackTimer += deltaTime;

	if (bRequestPath)
		return;


	switch (_state)
	{
	case MonsterState::MS_IDLE:
	{
		// idle ���¿�����
		_idleTime -= deltaTime;
		if (_idleTime <= 0)
		{
			// idle �ð��� �� �Ǿ�����
			if(SetRandomDestination())
			{
				_state = MonsterState::MS_MOVING;
			}
		}
	}
	break;

	case MonsterState::MS_MOVING:
	{
		bMoving = true;
		if (MoveToDestination(deltaTime))
		{
			//������ ����������
			_state = MonsterState::MS_IDLE;
			_idleTime = _defaultIdleTime;
		}
	}
	break;

	case MonsterState::MS_ATTACKING:
	{
		AttackPlayer(deltaTime);
	}
	break;

	case MonsterState::MS_CHASING:
	{
		bMoving = true;
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

bool Monster::MoveToDestination(float deltaTime)
{
	float dirX = _destination.X - _position.X;
	float dirY = _destination.Y - _position.Y;
	float distance = sqrt(dirX * dirX + dirY * dirY);

	if (distance < 10.0f)
	{
		// �������� ����������
		//_state = MonsterState::MS_IDLE;
		//_idleTime = 5.0f;
		return true;
	}
	else
	{
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

		{
			//���� ���
			uint16 newSectorY = _position.Y / _sectorYSize;
			uint16 newSectorX = _position.X / _sectorXSize;

			if (newSectorY >= 15 || newSectorX >= 15)
			{
				__debugbreak();
			}

			if (_currentSector->Y == newSectorY && _currentSector->X == newSectorX)
			{
				return false;
			}

			Sector* currentSector = _currentSector;
			Sector* newSector = GetField()->GetSector(newSectorY, newSectorX);
			ProcessSectorChange(newSector);
		}
	}
	return false;
	
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
				_rotation.Yaw = Util::CalculateRotation(_position, _targetPlayer->Position);
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
				SendPacket_Around(monsterSkilPacket);
				CPacket::Free(monsterSkilPacket);

				/*_PacketHandleThread->SendPacket_BroadCast(monsterSkilPacket);
				CPacket::Free(monsterSkilPacket);*/

				//������ ��Ŷ ����
				//���������� ������ ��Ŷ ��������
				CPacket* resDamagePacket = CPacket::Alloc();
				int32 AttackerType = TYPE_MONSTER;
				int64 AttackerID = _monsterInfo.MonsterID;
				int32 targetType = TYPE_PLAYER;
				int64 TargetID = _targetPlayer->playerInfo.PlayerID;
				int32 Damage = _damage;
				MP_SC_GAME_RES_DAMAGE(resDamagePacket, AttackerType, AttackerID, targetType, TargetID, Damage);
				_attackTimer = 0;
				SendPacket_Around(resDamagePacket);
				CPacket::Free(resDamagePacket);
			}
		}
		else {
			_state = MonsterState::MS_CHASING;
			//���⼭ ���� �� ��û�ϰ�
			FVector Destination = { _targetPlayer->Position.X, _targetPlayer->Position.Y, _position.Z };
			SetDestination(Destination);
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

			if (MoveToDestination(deltaTime))
			{
				printf("chaes player destination %d %d\n", _destination.X, _destination.Y);
				//printf("������ ����\n");
				//�������� ����������
				FVector newDestination{ _targetPlayer->Position.X, _targetPlayer->Position.Y, _position.Z };
				SetDestination(newDestination);
			}
			else
			{
				printf("chase player now : %d %d\n", _position.X, _position.Y);
			}
		}
	}
}


void Monster::SetDestination(FVector dest)
{
	Pos start = { (int)_position.Y, (int)_position.X };
	Pos end = { (int)dest.Y, (int)dest.X };

	if (start == end)
	{
		return;
	}

	//start�� end�� ��ֹ��̸� end���� ������ �̵�
	if (!GetField()->CheckValidPos(start) || !GetField()->CheckValidPos(end))
	{
		_destination = dest;
		_rotation.Yaw = Util::CalculateRotation(_position, _destination);
		SendMovePacket();
		return;
	}
	//_destination = dest;
	//TODO: Ŭ���̾�Ʈ�� ���� �̵� ��Ŷ ����
	bRequestPath = true;
	_requestPath.clear();
	GetField()->RequestMonsterPath(this, start, end);

}

bool Monster::SetRandomDestination()
{
	float range = 500.0f; // ���� ����

	FVector RandomDestination;
	RandomDestination.X = _position.X + (rand() % static_cast<int>(range * 2)) - range;
	RandomDestination.Y = _position.Y + (rand() % static_cast<int>(range * 2)) - range;
	RandomDestination.X = std::clamp(RandomDestination.X, (double)100, (double)GetField()->GetMapXSize() - 100);
	RandomDestination.Y = std::clamp(RandomDestination.Y, (double)100, (double)GetField()->GetMapYSize() - 100);

	if (!GetField()->CheckValidPos({ (int)_destination.Y, (int)_destination.X }))
	{
		return false;
	}

	SetDestination(RandomDestination);

	return true;
}

void Monster::HandleAsyncFindPath()
{
	_path.clear();
	_pathIndex = 0;
	_path = _requestPath;

	if (_path.size() > 0)
	{
		_destination = { (double)_path[0].x, (double)_path[0].y, _position.Z };
		_rotation.Yaw = Util::CalculateRotation(_position, _destination);
		bRequestPath = false;
		SendMovePacket();
	}
	else if (_path.size() == 0)
	{
		__debugbreak();
	}
}

bool Monster::TakeDamage(int damage, Player* attacker)
{
	bool bDeath = false;
	if (_targetPlayer != attacker)
	{
		// �ٸ���
		//�Ұ� ��������
		_chaseTime = 0.0f;
	}

	_monsterInfo.Hp -= damage;
	if (_monsterInfo.Hp > 0)
	{
		_targetPlayer = attacker;
		_state = MonsterState::MS_ATTACKING;
	}
	else {
		// ���� Ǯ�� �־���ϳ�?
		_state = MonsterState::MS_DEATH;
		bDeath = true;
	}
	return bDeath;
}

float Monster::GetDistanceToPlayer(Player* player)
{
	float dirX = player->Position.X - _position.X;
	float dirY = player->Position.Y - _position.Y;
	return sqrt(dirX * dirX + dirY * dirY);
}

void Monster::OnSpawn()
{
	//TODO: ���� ���� ��Ŷ ������
	CPacket* packet = CPacket::Alloc();
	MP_SC_SPAWN_MONSTER(packet, _monsterInfo, _position, _rotation);
	//printf("send monster spawn mosterID : %lld\n", monster->_monsterInfo.MonsterID);
	SendPacket_Around(packet);
	CPacket::Free(packet);
}

void Monster::SetTargetPlayerEmpty()
{
	_targetPlayer = nullptr;
	_state = MonsterState::MS_IDLE;
	_idleTime = _defaultIdleTime;

	//idle���¶�� ������ MonsterStop ��Ŷ
	CPacket* idlePacket = CPacket::Alloc();
	MP_SC_GAME_RES_MONSTER_STOP(idlePacket, _monsterInfo.MonsterID, _position, _rotation);
	SendPacket_Around(idlePacket);
	CPacket::Free(idlePacket);
}

void Monster::ProcessSectorChange(Sector* newSector)
{
	AddSector(newSector);
	RemoveSector(newSector);
	_currentSector = newSector;
}

void Monster::AddSector(Sector* newSector)
{
	// 1: new Sector�� �ִ� ���Ϳ� �� ĳ���� ����, �׼� ������
	Sector** addSector = nullptr;
	int8 addSectorNum = 0;
	int8 moveDirection = diffToDirection[{newSector->Y - _currentSector->Y, newSector->X - _currentSector->X}];

	// �̵� ���⿡ ���� �� ĳ���Ϳ��� ���� �߰��� ���� ���
	switch (moveDirection)
	{
	case MOVE_DIR_LL:
	{
		addSector = newSector->left;
		addSectorNum = newSector->leftSectorNum;
	}
	break;

	case MOVE_DIR_RR:
	{
		addSector = newSector->right;
		addSectorNum = newSector->rightSectorNum;
	}
	break;

	case MOVE_DIR_UU:
	{
		addSector = newSector->up;
		addSectorNum = newSector->upSectorNum;
	}
	break;

	case MOVE_DIR_DD:
	{
		addSector = newSector->down;
		addSectorNum = newSector->downSectorNum;
	}
	break;

	case MOVE_DIR_RU:
	{
		addSector = newSector->upRight;
		addSectorNum = newSector->upRightSectorNum;
	}
	break;

	case MOVE_DIR_LU:
	{
		addSector = newSector->upLeft;
		addSectorNum = newSector->upLeftSectorNum;
	}
	break;

	case MOVE_DIR_RD:
	{
		addSector = newSector->downRight;
		addSectorNum = newSector->downRightSectorNum;
	}
	break;

	case MOVE_DIR_LD:
	{
		addSector = newSector->downLeft;
		addSectorNum = newSector->downLeftSectorNum;
	}
	break;

	default:
	{
		__debugbreak();
	}
	}

	//���� �߰��� ���Ϳ�, �� ������ ����, �̵��� �̵� ��Ŷ ������
	CPacket* spawnMonsterPacket = CPacket::Alloc();
	MP_SC_SPAWN_MONSTER(spawnMonsterPacket, _monsterInfo, _position, _rotation);
	for (int i = 0; i < addSectorNum; i++)
	{
		SendPacket_Sector(addSector[i], spawnMonsterPacket);
	}
	CPacket::Free(spawnMonsterPacket);

	//�̵����̾����� �̵� ��Ŷ ������
	if (bMoving)
	{
		CPacket* monsterMovePacket = CPacket::Alloc();
		MP_SC_MONSTER_MOVE(monsterMovePacket, _monsterInfo.MonsterID, _destination, _rotation);
		for (int i = 0; i < addSectorNum; i++)
		{
			SendPacket_Sector(addSector[i], monsterMovePacket);
		}
		CPacket::Free(monsterMovePacket);
	}

	newSector->fieldObjectVector.push_back(this);
}

void Monster::RemoveSector(Sector* newSector)
{
	//���� ���Ϳ��� �� ĳ���� �����ϰ�
	Sector* nowSector = _currentSector;
	std::vector<FieldObject*>& fieldObjectVector = nowSector->fieldObjectVector;
	auto it = std::find(fieldObjectVector.begin(), fieldObjectVector.end(), this);
	if (it != fieldObjectVector.end())
	{
		fieldObjectVector.erase(it);
	}

	//���Ͱ� ����ʿ� ���� ����� ���� ���ϱ�
	Sector** deleteSector = nullptr;
	int8 deleteSectorNum = 0;
	int8 moveDirection = diffToDirection[{newSector->Y - _currentSector->Y, newSector->X - _currentSector->X}];

	switch (moveDirection)
	{
	case MOVE_DIR_LL:
	{
		deleteSector = nowSector->right;
		deleteSectorNum = nowSector->rightSectorNum;
	}
	break;

	case MOVE_DIR_RR:
	{
		deleteSector = nowSector->left;
		deleteSectorNum = nowSector->leftSectorNum;
	}
	break;

	case MOVE_DIR_UU:
	{
		deleteSector = nowSector->down;
		deleteSectorNum = nowSector->downSectorNum;
	}
	break;

	case MOVE_DIR_DD:
	{
		deleteSector = nowSector->up;
		deleteSectorNum = nowSector->upSectorNum;
	}
	break;

	case MOVE_DIR_RU:
	{
		deleteSector = nowSector->downLeft;
		deleteSectorNum = nowSector->downLeftSectorNum;
	}
	break;

	case MOVE_DIR_LU:
	{
		deleteSector = nowSector->downRight;
		deleteSectorNum = nowSector->downRightSectorNum;
	}
	break;

	case MOVE_DIR_RD:
	{
		deleteSector = nowSector->upLeft;
		deleteSectorNum = nowSector->upLeftSectorNum;
	}
	break;

	case MOVE_DIR_LD:
	{
		deleteSector = nowSector->upRight;
		deleteSectorNum = nowSector->upRightSectorNum;
	}
	break;

	default:
	{
		__debugbreak();
	}
	}

	//��� ���Ϳ� �� ĳ���� ���� ��Ŷ ������
	CPacket* despawnMonsterPacket = CPacket::Alloc();
	MP_SC_GAME_DESPAWN_MONSTER(despawnMonsterPacket, _monsterInfo.MonsterID);
	for (int i = 0; i < deleteSectorNum; i++)
	{
		SendPacket_Sector(deleteSector[i], despawnMonsterPacket);
	}
	CPacket::Free(despawnMonsterPacket);

}

void Monster::SendMovePacket()
{
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, _destination, _rotation);
	SendPacket_Around(packet);
	CPacket::Free(packet);
}


