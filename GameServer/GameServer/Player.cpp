#include "Player.h"
#include "Type.h"
#include "GameData.h"
using namespace std;


void Player::Init(int64 sessionId)
{
	_sessionId = sessionId;
	_lastRecvTime = 0;
	_bLogined = false;
	playerInfo = PlayerInfo();
	Position = { 0, 0,  PLAYER_Z_VALUE };
	playerInfos.clear();

}

void Player::Update(float deltaTime)
{
	if (bMoving)
	{
		Move(deltaTime);
	}
}

void Player::SetDestination(const FVector& NewDestination)
{
	_destination = NewDestination;
	bMoving = true;

}

void Player::StopMove()
{
    bMoving = false;
}

void Player::Move(float deltaTime) {
    FVector Direction = { _destination.X - Position.X, _destination.Y - Position.Y, 0 };
    double Distance = std::sqrt(Direction.X * Direction.X + Direction.Y * Direction.Y);
    FVector NormalizedDirection = { Direction.X / Distance, Direction.Y / Distance, 0 };

    if (Distance > 0.0) {
        double DistanceToMove = _speed * deltaTime;
        Position.X += NormalizedDirection.X * DistanceToMove;
        Position.Y += NormalizedDirection.Y * DistanceToMove;
    }

    // �̰� ���� ��ŭ�ؾ�����
    if (std::abs(Position.X - _destination.X) < 1.0 &&
        std::abs(Position.Y - _destination.Y) < 1.0) {
        Position = _destination; // �������� �����ߴٰ� ����
        bMoving = false;
    }

    double RotationAngleRadians = std::atan2(NormalizedDirection.Y, NormalizedDirection.X);
    double RotationAngleDegrees = RotationAngleRadians * 180 / PI; // ������ ���� �𸮾� degree
    Rotation.Yaw = RotationAngleDegrees;
}

bool Player::TakeDamage(int32 damage)
{
    //������ tru
    playerInfo.Hp -= damage;
    if (playerInfo.Hp <= 0)
    {
		playerInfo.Hp = 0;
		return true;
	}

    return false;
}

