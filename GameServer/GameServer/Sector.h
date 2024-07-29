#pragma once
#include "Type.h"
#include <unordered_set>
#include <vector>
#define AROUND_SEC_NUM 9
#define STRAIGHT_SEC_CHANGE_NUM 3
#define DIAGONAL_SEC_CHANGE_NUM 5
#define MOVE_DIR_LL			0
#define MOVE_DIR_LU			1
#define MOVE_DIR_UU			2
#define MOVE_DIR_RU			3
#define MOVE_DIR_RR			4
#define MOVE_DIR_RD			5
#define MOVE_DIR_DD			6
#define MOVE_DIR_LD			7
#define MOVE_DIR_MAX           8

class Player;

class Sector
{
public:
	uint8 X;
	uint8 Y;
	// �ֺ� 8�� + ����
	Sector* _around[AROUND_SEC_NUM];
	uint8 aroundSectorNum;
	// �����̵� 3��
	Sector* left[STRAIGHT_SEC_CHANGE_NUM];
	uint8 leftSectorNum;
	// �������̵� 3��
	Sector* right[STRAIGHT_SEC_CHANGE_NUM];
	uint8 rightSectorNum;
	// ���� �̵�3��
	Sector* up[STRAIGHT_SEC_CHANGE_NUM];
	uint8 upSectorNum;
	// �Ʒ� �̵� 3��
	Sector* down[STRAIGHT_SEC_CHANGE_NUM];
	uint8 downSectorNum;
	// �� ������ �̵� 5��
	Sector* upRight[DIAGONAL_SEC_CHANGE_NUM];
	uint8 upRightSectorNum;
	// �� ���� �̵� 5��
	Sector* upLeft[DIAGONAL_SEC_CHANGE_NUM];
	uint8 upLeftSectorNum;
	// �Ʒ� ������ �̵� 5��
	Sector* downRight[DIAGONAL_SEC_CHANGE_NUM];
	uint8 downRightSectorNum;
	// �Ʒ� ���� �̵� 5��
	Sector* downLeft[DIAGONAL_SEC_CHANGE_NUM];
	uint8 downLeftSectorNum;

	std::vector<Player*> playerVector;
};

