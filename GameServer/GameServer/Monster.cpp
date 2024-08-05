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
	// monsterId : 1�� ����
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
	_maxChaseTime = 5.0f;
	_aggroRange = 1000.0f;
	_damage = 5;
	_rotation = { 0,0,0 };
	_targetPlayer = nullptr;
}

Monster::Monster(FieldPacketHandleThread* field, uint16 objectType, uint16 monsterType, FVector spawnPosition) : FieldObject(field, objectType)
{
	// monsterId : 1�� ����
	_monsterInfo.MonsterID = _objectId;
	_monsterInfo.Type = monsterType;
	_monsterInfo.Hp = 100;
	_position = spawnPosition;
	_state = MonsterState::MS_IDLE;
	_speed = 200.0f;
	_destination.Z = 88.1;
	_idleTime = _defaultIdleTime;
	_attackRange = 200.0f;
	_attackCooldown = 5.0f;
	_chaseTime = 0;
	_maxChaseTime = 5.0f;
	_aggroRange = 1000.0f;
	_damage = 5;
	_rotation = { 0,0,0 };
	_targetPlayer = nullptr;
}

void  Monster::Update(float deltaTime)
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

	uint16 newSectorY = _position.Y / _sectorYSize;
	uint16 newSectorX = _position.X / _sectorXSize;

	if(newSectorY >= 50 || newSectorX >= 50)
	{
		__debugbreak();
	}

	if (_currentSector->Y == newSectorY && _currentSector->X == newSectorX)
	{
		return;
	}

	Sector* currentSector = _currentSector;
	Sector* newSector = GetField()->GetSector(newSectorY, newSectorX);
	ProcessSectorChange(newSector);
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

			//Destination�ϰ� ���� ������ ���̰� 200.f �Ʒ��� �׳� �̵��ϰ�
			//�ƴϸ� ��ã�� ��û
			if (length < 200.f)
			{
				SetDestination(Destination);
				MoveToDestination(deltaTime);
			}
			else {
				//��ã�� ��û
				//��ã�� ��û ��Ŷ ������
				GetField()->RequestAsyncJob();
				Pos start = { _position.X, _position.Y };
				Pos end = { Destination.X, Destination.Y };

				//jps->FindFirstPath �̷��͵� ������ ���ڴµ�
				GetField()->RequestAsyncJob(GetObjectId(),
					[start, end, this ,findedPath]()
					{
						this->_jps->FindPath(start, end, player->_path);
					}
				);


				CPacket* packet = CPacket::Alloc();
				MP_SC_GAME_REQ_FIND_PATH(packet, _monsterInfo.MonsterID, _position, Destination);
				SendPacket_Around(packet);
				CPacket::Free(packet);
			}


			//SetDestination(Destination);
			//MoveToDestination(deltaTime);
		}
	}
}


void Monster::SetDestination(FVector dest)
{
	_destination = dest;
	//TODO: Ŭ���̾�Ʈ�� ���� �̵� ��Ŷ ����
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, dest, _rotation);
	SendPacket_Around(packet);
	CPacket::Free(packet);
	_rotation.Yaw = Util::CalculateRotation(_position, _destination);

}

void Monster::SetRandomDestination()
{
	//TODO: ���� ���� ��ġ �������
	// ���� ������ ���Ѵ�
	float range = REQUEST_FIND_PATH_THRESHOLD; // ���� ����
	_destination.X = _position.X + (rand() % static_cast<int>(range * 2)) - range;
	_destination.Y = _position.Y + (rand() % static_cast<int>(range * 2)) - range;

	uint32 mapXSize = GetField()->GetMapXSize();
	uint32 mapYSize = GetField()->GetMapYSize();

	_destination.X = std::clamp(_destination.X, (double)100, double(mapXSize) - 100);
	_destination.Y = std::clamp(_destination.Y, (double)100, double(mapYSize) - 100);
	_rotation.Yaw = Util::CalculateRotation(_position, _destination);


	//���⼭���� �̵��ҰŴϱ� �̵���Ŷ ������ �ǳ�
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, _destination, _rotation);
	SendPacket_Around(packet);
	CPacket::Free(packet);
}

bool Monster::TakeDamage(int damage, Player* attacker)
{
	bool bDeath = false;
	if (_targetPlayer!= attacker)
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
	if (_state == MonsterState::MS_MOVING)
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
