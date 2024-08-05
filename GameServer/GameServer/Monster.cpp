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
	// monsterId : 1씩 증가
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
	// monsterId : 1씩 증가
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
	//여기서 하는게 좋은 설계인가
	// 공격 쿨타임 계산
	_attackTimer += deltaTime;

	switch(_state)
	{
		case MonsterState::MS_IDLE:
		{
			// idle 상태였으면
			_idleTime -= deltaTime;
			if(_idleTime <= 0)
			{
				// idle 시간이 다 되었으면
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
			//TODO: 얘 아무것도 할거 없고
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
		// 목적지에 도착했으면
		_state = MonsterState::MS_IDLE;
		_idleTime = 5.0f;
	}
	else
	{
		// 목적지로 이동

		// 속도 * deltaTime
		float moveDist = _speed * deltaTime;
		if (moveDist > distance)
		{
			moveDist = distance;
		}


		// 이동거리 * x방향 / 거리
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

// 캐릭터가 나가버리거나 하면 이 targetPlayer를 들고있던 애는 어떻게해야하는거지
void Monster::AttackPlayer(float deltaTime)
{
	if (_targetPlayer)
	{
		float distance = GetDistanceToPlayer(_targetPlayer);
		if (distance <= _attackRange)
		{
			//공격범위안에들어왔으면
			// 추적 성공
			//추적 시간 초기화
			_chaseTime = 0.f;

			if (_attackTimer >= _attackCooldown)
			{
				_rotation.Yaw = Util::CalculateRotation(_position, _targetPlayer->Position);
				//일단 Stop을 해야하네
				
				//Stop Packet도 그냥 보냅시다
	/*			CPacket*  monsterStopPacket = CPacket::Alloc();
				MP_SC_GAME_RES_MONSTER_STOP(monsterStopPacket, _monsterInfo.MonsterID, _position, _rotation);
				_GuardianFieldThread->SendPacket_BroadCast(monsterStopPacket);
				CPacket::Free(monsterStopPacket);*/

				//여기서 공격하면 된다
				//클라이언트에 몬스터 공격 애니메이션 전송 및
				//근데 로테이션을 캐릭터를 향해서 가져와야하네
				CPacket* monsterSkilPacket = CPacket::Alloc();
				int32 SkillID = 1;
				MP_SC_GAME_RES_MONSTER_SKILL(monsterSkilPacket, _monsterInfo.MonsterID, _position, _rotation, SkillID);
				SendPacket_Around(monsterSkilPacket);
				CPacket::Free(monsterSkilPacket);

				/*_PacketHandleThread->SendPacket_BroadCast(monsterSkilPacket);
				CPacket::Free(monsterSkilPacket);*/
				
				//데미지 패킷 전송
				//공격했으면 데미지 패킷 보내여지
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
			//idle상태라고 보내기 MonsterStop 패킷
			/*CPacket* idlePacket = CPacket::Alloc();
			MP_SC_GAME_RES_MONSTER_STOP(idlePacket, _monsterInfo.MonsterID, _position, _rotation);
			_GuardianFieldThread->SendPacket_BroadCast(idlePacket);
			CPacket::Free(idlePacket)*/;

			// maxChaseTime 초과하면
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
			//플에이어 몬스터 사이 거리
			float dirX = _targetPlayer->Position.X - _position.X;
			float dirY = _targetPlayer->Position.Y - _position.Y;
			float length = sqrt(dirX * dirX + dirY * dirY);

			//정규화 하고
			dirX /= length;
			dirY /= length;

			// attackRagne곱해서
			FVector Destination{
				_targetPlayer->Position.X - dirX * (_attackRange - 10), // 범위 안에서
				_targetPlayer->Position.Y - dirY * (_attackRange - 10),
				_position.Z
			};

			//Destination하고 현재 포지션 차이가 200.f 아래면 그냥 이동하고
			//아니면 길찾기 요청
			if (length < 200.f)
			{
				SetDestination(Destination);
				MoveToDestination(deltaTime);
			}
			else {
				//길찾기 요청
				//길찾기 요청 패킷 보내기
				GetField()->RequestAsyncJob();
				Pos start = { _position.X, _position.Y };
				Pos end = { Destination.X, Destination.Y };

				//jps->FindFirstPath 이런것도 넣으면 좋겠는데
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
	//TODO: 클라이언트에 몬스터 이동 패킷 전송
	CPacket* packet = CPacket::Alloc();
	MP_SC_MONSTER_MOVE(packet, _monsterInfo.MonsterID, dest, _rotation);
	SendPacket_Around(packet);
	CPacket::Free(packet);
	_rotation.Yaw = Util::CalculateRotation(_position, _destination);

}

void Monster::SetRandomDestination()
{
	//TODO: 현재 몬스터 위치 기반으로
	// 랜덤 목적지 정한다
	float range = REQUEST_FIND_PATH_THRESHOLD; // 범위 설정
	_destination.X = _position.X + (rand() % static_cast<int>(range * 2)) - range;
	_destination.Y = _position.Y + (rand() % static_cast<int>(range * 2)) - range;

	uint32 mapXSize = GetField()->GetMapXSize();
	uint32 mapYSize = GetField()->GetMapYSize();

	_destination.X = std::clamp(_destination.X, (double)100, double(mapXSize) - 100);
	_destination.Y = std::clamp(_destination.Y, (double)100, double(mapYSize) - 100);
	_rotation.Yaw = Util::CalculateRotation(_position, _destination);


	//여기서부터 이동할거니까 이동패킷 보내면 되나
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
		// 다르면
		//할게 뭐가있지
		_chaseTime = 0.0f;
	}

	_monsterInfo.Hp -= damage;
	if (_monsterInfo.Hp > 0)
	{
		_targetPlayer = attacker;
		_state = MonsterState::MS_ATTACKING;
	}
	else {
		// 몬스터 풀에 넣어야하나?
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
	//TODO: 몬스터 스폰 패킷 날리기
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

	//idle상태라고 보내기 MonsterStop 패킷
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
	// 1: new Sector에 있는 섹터에 이 캐릭터 생성, 액션 보내고
	Sector** addSector = nullptr;
	int8 addSectorNum = 0;
	int8 moveDirection = diffToDirection[{newSector->Y - _currentSector->Y, newSector->X - _currentSector->X}];

	// 이동 방향에 따라 이 캐릭터에게 새로 추가된 섹터 계산
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

	//새로 추가된 섹터에, 이 몬스터의 생성, 이동시 이동 패킷 보내기
	CPacket* spawnMonsterPacket = CPacket::Alloc();
	MP_SC_SPAWN_MONSTER(spawnMonsterPacket, _monsterInfo, _position, _rotation);
	for (int i = 0; i < addSectorNum; i++)
	{
		SendPacket_Sector(addSector[i], spawnMonsterPacket);
	}
	CPacket::Free(spawnMonsterPacket);

	//이동중이었으면 이동 패킷 보내기
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
	//먼저 섹터에서 이 캐릭터 삭제하고
	Sector* nowSector = _currentSector;
	std::vector<FieldObject*>& fieldObjectVector = nowSector->fieldObjectVector;
	auto it = std::find(fieldObjectVector.begin(), fieldObjectVector.end(), this);
	if (it != fieldObjectVector.end())
	{
		fieldObjectVector.erase(it);
	}

	//섹터가 변경됨에 따라 사라진 섹터 구하기
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

	//벗어난 섹터에 이 캐릭터 삭제 패킷 보내고
	CPacket* despawnMonsterPacket = CPacket::Alloc();
	MP_SC_GAME_DESPAWN_MONSTER(despawnMonsterPacket, _monsterInfo.MonsterID);
	for (int i = 0; i < deleteSectorNum; i++)
	{
		SendPacket_Sector(deleteSector[i], despawnMonsterPacket);
	}
	CPacket::Free(despawnMonsterPacket);
	
}
