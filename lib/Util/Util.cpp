
#include "Util.h"


double Util::CalculateRotation(const FVector& oldPosition, const FVector& newPosition)
{
	double yaw = 0.0;

	double dx = newPosition.X - oldPosition.X;
	double dy = newPosition.Y - oldPosition.Y;
	if (dx != 0 || dy != 0) {
		// �������� ���� ��쿡�� calculate rotation
		yaw = std::atan2(dy, dx) * 180 / PI; // ���ȿ��� ��, �𸮾� degree
	}

	return yaw;
}	