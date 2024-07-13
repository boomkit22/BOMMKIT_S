#include "Player.h"
#include "Type.h"

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

void Player::Move(float deltaTime) {
    FVector Direction = { _destination.X - Position.X, _destination.Y - Position.Y, 0 };
    double Distance = std::sqrt(Direction.X * Direction.X + Direction.Y * Direction.Y);

    if (Distance > 0.0) {
        FVector NormalizedDirection = { Direction.X / Distance, Direction.Y / Distance, 0 };
        double DistanceToMove = _speed * deltaTime;
        Position.X += NormalizedDirection.X * DistanceToMove;
        Position.Y += NormalizedDirection.Y * DistanceToMove;
    }

    // 목적지에 도달했는지 확인 (예: 일정 거리 이내일 경우)
    if (std::abs(Position.X - _destination.X) < 1.0 &&
        std::abs(Position.Y - _destination.Y) < 1.0) {
        Position = _destination; // 목적지에 도달했다고 간주
        bMoving = false;
    }

}

