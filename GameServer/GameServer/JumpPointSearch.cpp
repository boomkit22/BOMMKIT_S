#include "JumpPointSearch.h"
using namespace std;
#define DIRECTION_NUM 8


Pos dir[DIRECTION_NUM] = {
	Pos{-1,0}, // �� 
	Pos{0, 1},  //������
	Pos{1, 0}, // �Ʒ�
	Pos{0, -1}, // ����
	Pos{-1,1},// ��������
	Pos {1,1}, // ������ �Ʒ�
	Pos{1,-1}, //���ʾƷ�
	Pos{-1,-1} // ������
};

int gVal[DIRECTION_NUM] = {
	10, // ��
	10, // ������
	10, // �Ʒ�
	10, // ����
	14, // �밢 ��������
	14, // �밢 ������ �Ʒ�
	14, //�밢  ���ʾƷ�
	14// �밢 ������ 
};

#define df_DIR_U 0
#define df_DIR_R 1
#define df_DIR_D 2
#define df_DIR_L 3
#define df_DIR_UR 4
#define df_DIR_RD 5
#define df_DIR_LD 6
#define df_DIR_LU 7

JumpPointSearch::JumpPointSearch(uint8** map, int32 mapYSize, int32 mapXSize)
{
	_originMap = map;
	_mapYSize = mapYSize;
	_mapXSize = mapXSize;

	_jpsMap = new uint8* [_mapYSize];
	for (int i = 0; i < _mapYSize; i++)
	{
		_jpsMap[i] = new uint8[_mapXSize];
		memset(_jpsMap[i], 0, sizeof(uint8) * _mapXSize);
	}

}

JumpPointSearch::~JumpPointSearch()
{

}

std::vector<Pos> JumpPointSearch::FindPath(Pos start, Pos end)
{
	std::vector<Pos> path;
	//First : �� �ʱ�ȭ�ϰ�
	memcpy(_jpsMap, _originMap, sizeof(uint8) * _mapYSize * _mapXSize);
	Node* startNode = CreateStartNode(_start);
	//����������  8���� �˻��ϰ�
	for (int i = 0; i < DIRECTION_NUM; i++)
	{
		CheckAndMakeCorner(startNode, _start, i);
	}
	delete startNode;
	
	if (openList.size() == 0)
	{
		//�� ��ã��
		return path;
	}

	while (true)
	{
		//���� �ϰ�
		openList.sort(NodeComparator());
		Node* nodeNow = openList.front();
		Pos posNow = nodeNow->_pos;

		if (posNow == _dest)
		{
			//���� Node�� �������̸�
			while (true)
			{
				path.push_back(nodeNow->_pos);
				if (nodeNow->_pos == _start)
				{
					break;
				}
				nodeNow = nodeNow->_parent;
			}
			std::reverse(path.begin(), path.end());
			vector<Pos> shortestPath;
			FindShortestPath(path, shortestPath);

			return shortestPath;
		}
		// �ƴϸ�
		openList.pop_front();

		for (int i = 0; i < 8; i++)
		{	
			if (nodeNow->directionArr[i] == 0)
			{
				continue;
			}
			// �̹� ����� Ž���������� Ž��
			if (CheckAndMakeCorner(nodeNow, posNow, i))
				break;
		}
		SetMap(posNow, CLOSE);
	}



	
}

Node* JumpPointSearch::CreateStartNode(Pos pos)
{
	int h = GetH(_start, _dest);
	Node* startNode = new Node;
	startNode->_pos = _start;
	startNode->_f = h;
	startNode->_h = h;

	//openList.push_back(startNode); push back ���ϰ�
	//_debugNodeList[_start] = startNode;
	SetMap(_start, OPEN);
	//_first = false;
	return startNode;
}

void JumpPointSearch::SetMap(Pos& pos, uint8 value)
{
	//if (pos.y < 0 || pos.x < 0 || pos.y >= _mapYSize || pos.x >= _mapXSize)
	//{
	//	return;
	//}

	//if (_started)
	//{
	//	if (pos == _start || pos == _dest)
	//	{
	//		return;
	//	}
	//}

	//if (value == BRESENHAM)
	//{
	//	tile[pos.y][pos.x] = value;
	//	return;
	//}

	//if (value != ROUTE)
	//{
	//	//��ΰ� �ƴѰ���
	//	// �����ΰ��� close���ƴѰ����� �����Ϸ��ϸ�
	//	if (GetMapValue(pos) == OPEN && value != CLOSE)
	//	{
	//		return;
	//	}

	//	// close�ΰ��� �����Ϸ��ϸ�
	//	if (GetMapValue(pos) == CLOSE)
	//	{
	//		return;
	//	}
	//}

	_jpsMap[pos.y][pos.x] = value;
}

int JumpPointSearch::CheckUp(Pos pos)
{
	// ���� ��ĭ�ö�°���
	int ret = 0;

	if (pos.y == 0)
	{
		return ret;
	}

	if (pos.x >= 1)
	{
		// ���� OBSTACLE, �������� NONE
		if (GetMapValue(pos + dir[df_DIR_L]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LU]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LU;
			ret |= 1 << df_DIR_U;
		}
	}

	if (pos.x <= _mapXSize - 2)
	{
		// �������� OBSTACLE�̰� ���������� NONE
		if (GetMapValue(pos + dir[df_DIR_R]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_UR]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_UR;
			ret |= 1 << df_DIR_U;
		}
	}

	return ret;
}

int JumpPointSearch::CheckRight(Pos pos)
{
	int ret = 0;

	if (pos.x == _mapXSize - 1)
	{
		return ret;
	}

	if (pos.y >= 1)
	{
		// ���� OBSTACLE�̰� �� �������� NONE
		if (GetMapValue(pos + dir[df_DIR_U]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_UR]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_UR;
			ret |= 1 << df_DIR_R;
		}
	}

	if (pos.y <= _mapYSize - 2)
	{
		// �Ʒ��� OBSTACLE�̰� �����ʾƷ��� NONE
		if (GetMapValue(pos + dir[df_DIR_D]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_RD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_RD;
			ret |= 1 << df_DIR_R;
		}
	}

	return ret;
}

int JumpPointSearch::CheckDown(Pos pos)
{
	int ret = 0;

	if (pos.y == _mapYSize - 1)
	{
		return ret;
	}

	if (pos.x >= 1)
	{
		// ������ obstacle�̰� ���ʾƷ��� NONE
		if (GetMapValue(pos + dir[df_DIR_L]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LD;
			ret |= 1 << df_DIR_D;
		}
	}

	if (pos.x <= _mapYSize - 2)
	{
		// �������� obstacle�̰� �����ʾƷ��� NONE
		if (GetMapValue(pos + dir[df_DIR_R]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_RD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_RD;
			ret |= 1 << df_DIR_D;
		}

	}

	return ret;
}

int JumpPointSearch::CheckLeft(Pos pos)
{
	int ret = false;

	if (pos.x == 0)
	{
		return ret;
	}

	if (pos.y >= 1)
	{
		// ������ OBSTACLE, �������� NONE
		if (GetMapValue(pos + dir[df_DIR_U]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LU]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LU;
			ret |= 1 << df_DIR_L;

		}
	}

	if (pos.y <= _mapYSize - 2)
	{
		// �Ʒ��� OBSTACLE, ���ʾƷ��� NONE
		if (GetMapValue(pos + dir[df_DIR_D]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LD;
			ret |= 1 << df_DIR_L;
		}
	}
	return ret;
}

int JumpPointSearch::CheckUpRight(Pos pos)
{
	int ret = 0;

	if (pos.x <= _mapXSize - 2)
	{
		// �Ʒ��� OBSTACLE�̰� �����ʾƷ� NONE
		if (GetMapValue(pos + dir[df_DIR_D]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_RD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_RD;
			ret |= 1 << df_DIR_UR;
		}
	}

	if (pos.y >= 1)
	{
		// �ަU�� OBSTACLE�̰� �������� NONE
		if (GetMapValue(pos + dir[df_DIR_L]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LU]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LU;
			ret |= 1 << df_DIR_UR;
		}

	}

	return ret;
}

int JumpPointSearch::CheckRightDown(Pos pos)
{
	int ret = 0;

	if (pos.x <= _mapXSize - 2)
	{
		// ������ OBSTACLE�̰� ���������� NONE
		if (GetMapValue(pos + dir[df_DIR_U]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_UR]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_UR;
			ret |= 1 << df_DIR_RD;

		}
	}

	if (pos.y >= 1)
	{
		// ������ OBSTACLE�̰� ���ʾƷ��� NONE
		if (GetMapValue(pos + dir[df_DIR_L]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LD;
			ret |= 1 << df_DIR_RD;
		}
	}

	return ret;
}

int JumpPointSearch::CheckLeftDown(Pos pos)
{
	int ret = false;
	if (pos.x >= 1)
	{
		// ������ OBSTACLE�̰� �������� NONE
		if (GetMapValue(pos + dir[df_DIR_U]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LU]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LU;
			ret |= 1 << df_DIR_LD;

		}
	}


	if (pos.y <= _mapYSize - 2)
	{
		// �������� OBSTACLE�̰� ������ �Ʒ��� NONE
		if (GetMapValue(pos + dir[df_DIR_R]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_RD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_RD;
			ret |= 1 << df_DIR_LD;
		}
	}
	return ret;
}

int JumpPointSearch::CheckLeftUp(Pos pos)
{
	int ret = 0;

	if (pos.y >= 1)
	{
		// �������� OBSTACLE�̰� ���������� NONE
		if (GetMapValue(pos + dir[df_DIR_R]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_UR]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_UR;
			ret |= 1 << df_DIR_LU;
		}
	}

	if (pos.x >= 1)
	{
		// �Ʒ��� OBSTACLE�̰� ���ʾƷ��� NONE
		if (GetMapValue(pos + dir[df_DIR_L]) == OBSTACLE && GetMapValue(pos + dir[df_DIR_LD]) != OBSTACLE)
		{
			ret |= 1 << df_DIR_LD;
			ret |= 1 << df_DIR_LU;
		}

	}

	return ret;
}

bool JumpPointSearch::CheckAndMakeCorner(Node* p, Pos pos, int direction)
{
	Pos originPos;
	originPos = pos;
	//if (pos.y < 0 || pos.x < 0 || pos.x >= _mapXSize || pos.y >= _mapYSize)
	//{
	//	__debugbreak();
	//}

	//if (GetMapValue(pos) == OBSTACLE)
	//{
	//	__debugbreak();
	//}

	//if (GetMapValue(pos) == END)
	//{
	//	__debugbreak();
	//}
	switch (direction)
	{

	case df_DIR_U:
	{
		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_U];
			distance += 10;
			if (pos.y < 0)
			{
				break;
			}

			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}

			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}

			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}

			//SetMap(pos, _checkValue);
			int dirVal = CheckUp(pos);
			if (!dirVal)
			{
				continue;
			}
			else
			{
				CreateNode(pos, p, dirVal, distance);
				break;
			}
		}
	}
	break;

	case df_DIR_R:
	{

		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_R];
			distance += 10;

			if (pos.x >= _mapXSize)
			{
				break;
			}


			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}

			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}

			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}


			//SetMap(pos, _checkValue);

			int dirVal = CheckRight(pos);
			if (!dirVal)
			{
				continue;
			}
			else
			{
				CreateNode(pos, p, dirVal, distance);
				break;
			}
		}
	}
	break;

	case df_DIR_D:
	{

		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_D];
			distance += 10;



			if (pos.y >= _mapYSize)
			{
				break;
			}

			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}
			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}


			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}

			//SetMap(pos, _checkValue);


			int dirVal = CheckDown(pos);
			if (!dirVal)
			{
				continue;
			}
			else
			{
				CreateNode(pos, p, dirVal, distance);
				break;
			}
		}
	}
	break;

	case df_DIR_L:
	{

		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_L];
			distance += 10;

			if (pos.x < 0)
			{
				break;
			}

			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}

			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}


			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}

			//SetMap(pos, _checkValue);


			int dirVal = CheckLeft(pos);
			if (!dirVal)
			{
				continue;
			}
			else
			{
				CreateNode(pos, p, dirVal, distance);
				break;

			}
		}
	}
	break;


	case df_DIR_UR:
	{
		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_UR];

			distance += 14;

			if (pos.x >= _mapXSize || pos.y < 0)
			{
				break;
			}

			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}

			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}

			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}


			//SetMap(pos, _checkValue);


			int dirVal = CheckUpRight(pos);

			// node ��������ִ°�
			Node* nodeHere = nullptr;
			if (dirVal != 0)
			{
				nodeHere = CreateNode(pos, p, dirVal, distance);
			}



			//pos�� ������ġ�̰�
			{
				Pos toUpPos = pos;
				int toUpDistance = 0;
				/*****************************************************
				*
				*                 �� �� �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					toUpPos = toUpPos + dir[df_DIR_U];

					toUpDistance += 10;
					if (toUpPos.y < 0)
					{
						break;
					}

					if (GetMapValue(toUpPos) == OBSTACLE)
					{
						break;
					}

					if (GetMapValue(toUpPos) == CLOSE)
					{
						continue;
					}


					if (toUpPos == _dest)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_UR, distance);
						}
						// �������� ��� �����
						CreateNode(toUpPos, nodeHere, 0, toUpDistance);
						return true;
					}

					//SetMap(toUpPos, _checkValue);
					int upDirVal = CheckUp(toUpPos);

					if (upDirVal != 0)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_UR, distance);
						}

						CreateNode(toUpPos, nodeHere, upDirVal, toUpDistance);
						break;

					}
				}
				// ���� ���� ���� ��
			}



			{
				Pos toRightPos = pos;
				int toRightDistance = 0;
				/*****************************************************
				*
				*                 ������ �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					toRightPos = toRightPos + dir[df_DIR_R];

					toRightDistance += 10;
					if (toRightPos.x >= _mapXSize)
					{
						break;
					}

					if (GetMapValue(toRightPos) == OBSTACLE)
					{
						break;
					}

					if (GetMapValue(toRightPos) == CLOSE)
					{
						continue;
					}



					if (toRightPos == _dest)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_UR, distance);
						}
						CreateNode(toRightPos, nodeHere, 0, toRightDistance);
						return true;
					}

					//SetMap(toRightPos, _checkValue);

					int rightDirVal = CheckRight(toRightPos);

					if (rightDirVal != 0)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_UR, distance);
						}

						CreateNode(toRightPos, nodeHere, rightDirVal, toRightDistance);
						break;
					}
				}
				// ������ ���� ���� ��
			}

			if (nodeHere != nullptr)
			{
				break;
			}
		}
	}
	break;

	case df_DIR_RD:
	{
		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_RD];

			distance += 14;
			if (pos.x >= _mapXSize || pos.y >= _mapYSize)
			{

				break;
			}

			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}

			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}

			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}

			//SetMap(pos, _checkValue);

			int dirVal = CheckRightDown(pos);


			// node ��������ִ°�
			Node* nodeHere = nullptr;
			if (dirVal != 0)
			{
				nodeHere = CreateNode(pos, p, dirVal, distance);
			}
			//checkValue�� 0�̾ƴϾ����� node�� ����ǥ�� �����Ȳ�ε�?


			//pos�� ������ġ�̰�
			{
				Pos toDownPos = pos;
				int toDownDistance = 0;
				/*****************************************************
				*
				*                 �Ʒ� �� �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					toDownPos = toDownPos + dir[df_DIR_D];

					toDownDistance += 10;
					if (toDownPos.y >= _mapYSize)
					{
						break;
					}

					if (GetMapValue(toDownPos) == OBSTACLE)
					{
						break;
					}


					if (GetMapValue(toDownPos) == CLOSE)
					{
						continue;
					}

					if (toDownPos == _dest)
					{

						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_RD, distance);
						}
						// �������� ��� �����
						CreateNode(toDownPos, nodeHere, 0, toDownDistance);
						return true;
					}

					//SetMap(toDownPos, _checkValue);


					int downDirVal = CheckDown(toDownPos);

					if (downDirVal != 0)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_RD, distance);
						}

						CreateNode(toDownPos, nodeHere, downDirVal, toDownDistance);
						break;
					}
				}
				// ���� ���� ���� ��
			}




			{
				Pos toRightPos = pos;
				int toRightDistance = 0;
				/*****************************************************
				*
				*                 ������ �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					// ������ǥ�� ���� ���������� ����
					toRightPos = toRightPos + dir[df_DIR_R];
					toRightDistance += 10;
					if (toRightPos.x >= _mapXSize)
					{
						break;
					}

					if (GetMapValue(toRightPos) == OBSTACLE)
					{
						break;
					}


					if (GetMapValue(toRightPos) == CLOSE)
					{
						continue;
					}


					if (toRightPos == _dest)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_RD, distance);
						}
						// �������� ��� �����
						CreateNode(toRightPos, nodeHere, 0, toRightDistance);
						return true;
					}

					//SetMap(toRightPos, _checkValue);

					int rightDirVal = CheckRight(toRightPos);

					if (rightDirVal != 0)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_RD, distance);
						}
						CreateNode(toRightPos, nodeHere, rightDirVal, toRightDistance);
						break;

					}
				}
				// ������ ���� ���� ��
			}
			if (nodeHere != nullptr)
			{
				break;
			}
		}
	}
	break;

	// ���� �Ʒ�
	case df_DIR_LD:
	{

		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_LD];
			distance += 14;
			if (pos.x < 0 || pos.y >= _mapYSize)
			{
				break;
			}

			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}

			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}


			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}

			//SetMap(pos, _checkValue);

			int dirVal = CheckLeftDown(pos);

			// node ��������ִ°�
			Node* nodeHere = nullptr;
			if (dirVal != 0)
			{
				nodeHere = CreateNode(pos, p, dirVal, distance);
			}
			//checkValue�� 0�̾ƴϾ����� node�� ����ǥ�� �����Ȳ�ε�?


			//pos�� ������ġ�̰�
			{
				Pos toDownPos = pos;
				int toDownDistance = 0;
				/*****************************************************
				*
				*                 �Ʒ� �� �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					toDownPos = toDownPos + dir[df_DIR_D];
					// �Ʒ�����������ε�
					toDownDistance += 10;
					if (toDownPos.y >= _mapYSize)
					{
						break;
					}

					if (GetMapValue(toDownPos) == OBSTACLE)
					{
						break;
					}

					if (GetMapValue(toDownPos) == CLOSE)
					{
						continue;
					}

					if (toDownPos == _dest)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LD, distance);
						}
						// �������� ��� �����
						CreateNode(toDownPos, nodeHere, 0, toDownDistance);
						return true;
					}

					//SetMap(toDownPos, _checkValue);


					int downDirVal = CheckDown(toDownPos);

					if (downDirVal != 0)
					{

						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LD, distance);
						}
						CreateNode(toDownPos, nodeHere, downDirVal, toDownDistance);
						break;
					}
				}
				// �Ʒ��� ���� ���� ��
			}




			{
				Pos toLeftPos = pos;
				int toLeftDistance = 0;
				/*****************************************************
				*
				*                 ����  �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					toLeftPos = toLeftPos + dir[df_DIR_L];

					toLeftDistance += 10;
					if (toLeftPos.x < 0)
					{
						break;
					}

					if (GetMapValue(toLeftPos) == OBSTACLE)
					{
						break;
					}

					if (GetMapValue(toLeftPos) == CLOSE)
					{
						continue;
					}



					if (toLeftPos == _dest)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LD, distance);
						}
						// �������� ��� �����
						CreateNode(toLeftPos, nodeHere, 0, toLeftDistance);
						return true;
					}

					//SetMap(toLeftPos, _checkValue);

					int leftDirVal = CheckLeft(toLeftPos);

					if (leftDirVal != 0)
					{

						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LD, distance);
						}
						CreateNode(toLeftPos, nodeHere, leftDirVal, toLeftDistance);
						break;
					}
				}
				//���� ������� ��
			}
			if (nodeHere != nullptr)
			{
				break;
			}
		}
	}
	break;

	case df_DIR_LU:
	{

		int distance = 0;
		while (true)
		{
			pos = pos + dir[df_DIR_LU];
			distance += 14;
			if (pos.x < 0 || pos.y < 0)
			{
				break;
			}

			if (GetMapValue(pos) == OBSTACLE)
			{
				break;
			}


			if (GetMapValue(pos) == CLOSE)
			{
				continue;
			}

			if (pos == _dest)
			{
				CreateNode(pos, p, 0, distance);
				return true;
			}

			//SetMap(pos, _checkValue);
			int dirVal = CheckLeftUp(pos);

			Node* nodeHere = nullptr;
			if (dirVal != 0)
			{
				nodeHere = CreateNode(pos, p, dirVal, distance);
			}
			//checkValue�� 0�̾ƴϾ����� node�� ����ǥ�� �����Ȳ�ε�?


			//pos�� ������ġ�̰�
			{
				Pos toUpPos = pos;
				int toUpDistance = 10;
				/*****************************************************
				*
				*                �� �� �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					toUpPos = toUpPos + dir[df_DIR_U];
					// �Ʒ�����������ε�
					toUpDistance += 10;
					if (toUpPos.y < 0)
					{
						break;
					}

					if (GetMapValue(toUpPos) == OBSTACLE)
					{
						break;
					}

					if (GetMapValue(toUpPos) == CLOSE)
					{
						continue;
					}
					//SetMap(toUpPos, _checkValue);


					if (toUpPos == _dest)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LU, distance);
						}
						// �������� ��� �����
						CreateNode(toUpPos, nodeHere, 0, toUpDistance);
						return true;
					}

					int upDirVal = CheckUp(toUpPos);

					if (upDirVal != 0)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LU, distance);
						}
						CreateNode(toUpPos, nodeHere, upDirVal, toUpDistance);
						break;
					}
				}
				// �Ʒ��� ���� ���� ��
			}




			{
				Pos toLeftPos = pos;
				int toLeftDistance = 0;
				/*****************************************************
				*
				*                 ����  �� �� �� ��
				*
				* ****************************************************/
				while (true)
				{
					toLeftPos = toLeftPos + dir[df_DIR_L];

					toLeftDistance += 10;
					if (toLeftPos.x < 0)
					{

						break;
					}

					if (GetMapValue(toLeftPos) == OBSTACLE)
					{
						break;
					}

					if (GetMapValue(toLeftPos) == CLOSE)
					{
						continue;
					}

					//SetMap(toLeftPos, _checkValue);

					if (toLeftPos == _dest)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LU, distance);
						}
						// �������� ��� �����
						CreateNode(toLeftPos, nodeHere, 0, toLeftDistance);
						return true;

					}

					int leftDirVal = CheckLeft(toLeftPos);

					if (leftDirVal != 0)
					{
						if (nodeHere == nullptr)
						{
							nodeHere = CreateNode(pos, p, 1 << df_DIR_LU, distance);
						}
						CreateNode(toLeftPos, nodeHere, leftDirVal, toLeftDistance);
						break;
					}
				}
				//���� ������� ��
			}
			if (nodeHere != nullptr)
			{
				break;
			}
		}
	}
	break;
	}
	return false;
}

Node* JumpPointSearch::CreateNode(Pos pos, Node* parent, int direction, int distanceFromParent)
{
	// Ŭ������ �� ���� -> return
	if (GetMapValue(pos) == CLOSE)
	{
		return nullptr;
	}
	else {
		//open�ΰ� ���� -> �з�Ʈ���ؼ� �ٲٰ� return
		if (GetMapValue(pos) == OPEN)
		{
			auto it = openList.begin();
			for (; it != openList.end(); ++it)
			{
				if ((*it)->_pos.x == pos.x && (*it)->_pos.y == pos.y)
				{
					break;
				}
			}
			if (it == openList.end())
			{
				__debugbreak();
			}
			CheckAndChangeParent(parent, (*it), distanceFromParent);
			//ReopenNode((*it));
			return *it;
		}
	}

	Node* n = new Node;
	n->_pos = pos;
	n->_g = parent->_g + distanceFromParent;
	int h = GetH(pos, _dest);
	n->_h = h;
	n->_f = n->_g + h;
	n->_parent = parent;

	for (int i = 0; i < 8; i++)
	{
		if ((1 << i) & direction)
			n->AddDirection(i);

	}
	//_debugNodeList[pos] = n;
	openList.push_back(n);
	SetMap(pos, OPEN);
	return n;
}

void JumpPointSearch::ReopenNode(Node* n)
{
}

void JumpPointSearch::CheckAndChangeParent(Node* newParent, Node* exist, int distance)
{
}

int JumpPointSearch::GetH(Pos& pos, Pos& dest)
{
	return abs(dest.y - pos.y) * 10 + abs(dest.x - pos.x) * 10;
}

void JumpPointSearch::FindShortestPath(vector<Pos>& path, vector<Pos>& result)
{
	if (path.size() < 2)
	{
		__debugbreak();
	}

	//���۳��ְ�
	result.push_back(path[0]);

	int pathSize = (int)path.size();
	int nodeFromIndex = 0;
	int nodeToIndex = nodeFromIndex + 2;

	while (nodeToIndex < pathSize)
	{
		
		Pos nodeFromPos = path[nodeFromIndex];
		Pos nodeToPos = path[nodeToIndex];
		if (!CalculateBresenham(nodeFromPos, nodeToPos))
		{
			// ��ֹ� ������
			result.push_back(path[nodeToIndex - 1]);
			nodeFromIndex = nodeToIndex - 1;
			nodeToIndex = nodeFromIndex + 2;
		}
		else {
			// ��ֹ� ������
			nodeToIndex++;
		}
	}

	//������ ���ְ�
	result.push_back(path[pathSize - 1]);

	//// �ִ� ��� ������ ����
	//for (auto it = result.rbegin(); it != result.rend(); ++it)
	//{
	//	_debugShortestPath.push_back(*it);
	//}
}

bool JumpPointSearch::CalculateBresenham(Pos start, Pos end)
{
	int x2 = _dest.x;
	int y2 = _dest.y;
	int x1 = start.x;
	int y1 = start.y;
	// ����� �׳� start�� dest������ ���� _bresenhamNode�� �ְ�
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);

	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;

	int cumul = 0;

	if (dx >= dy)
	{
		while (true)
		{
			Pos pos{ y1, x1 };
			if (GetMapValue(pos) == OBSTACLE)
			{
				return false;
			}
			// ���� ���
			if (x1 == x2 && y1 == y2)
			{
				break;
			}
			x1 += sx;
			cumul += dy;
			int cumul2 = 2 * cumul;

			if (cumul2 > dx)
			{
				y1 += sy;
				cumul -= dx;
			}
		}
	}
	else {
		while (true)
		{
			Pos pos{ y1, x1 };
			if (GetMapValue(pos) == OBSTACLE)
			{
				return false;
			}
			// ���� ���
			if (x1 == x2 && y1 == y2)
			{
				break;
			}
			y1 += sy;
			cumul += dx;
			int cumul2 = 2 * cumul;

			if (cumul2 > dy)
			{
				x1 += sx;
				cumul -= dy;
			}
		}
	}

	return true;
}

