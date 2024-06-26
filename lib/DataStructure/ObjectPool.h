#pragma once
#include <new.h>
#include <stdlib.h>



template <class DATA, bool bPlacementNew>
class CObjectPool;

// bPlacementNew�� true�� ���� Ư��ȭ
template <class DATA>
class CObjectPool<DATA, true>
{

	struct st_BLOCK_NODE
	{
		DATA data;
		st_BLOCK_NODE* next;
	};

public:
	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CObjectPool()
	{
		// Alloc�� ������ / Free�� �ı��� ȣ�� ����

	}

	CObjectPool(int blockNum)
	{
		//TODO: iBlockNum��ŭ Alloc
		if (blockNum == 0)
		{
			//TODO: NOTHING
		}
		else {
			_freeNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
			_freeNode->next = nullptr;
			int i = 0;
			for (; i < blockNum - 1; i++)
			{
				st_BLOCK_NODE* node = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				node->next = _freeNode;
				_freeNode = node;
			}
		}

		_capacity += blockNum;
	}
	virtual	~CObjectPool()
	{
		//TODO: �Ҹ��ڿ��� �Ұ�
		// 1. ��ġ
		// 2. Ǯ�ȿ� �����ִ°͸� �Ÿ� ����
		// 3. �� �����ؼ� �޸� ����
	}


	DATA* Alloc(void)
	{
		_useCount++;
		if (_freeNode != nullptr)
		{
			// �ٰ� ������?
			DATA* ret = new(&_freeNode->data) DATA;
			_freeNode = _freeNode->next;

			return ret;

		}
		else
		{
			// �ٰž�����
			// ���� ��������
			_freeNode = new(st_BLOCK_NODE);
			DATA* ret = &_freeNode->data;
			_freeNode->next = nullptr;
			_freeNode = nullptr;

			// ���� ������� ���� capacity++9;
			_capacity++;

			return ret;
		}
	}


	void	Free(DATA* pData)
	{
		_useCount--;
		// �Ҹ��� ȣ�� �ϰ�
		pData->~DATA();
		((st_BLOCK_NODE*)pData)->next = _freeNode;
		_freeNode = (st_BLOCK_NODE*)pData;


		return;
	}


	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) {
		return _useCount;
	}


	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.
private:
	st_BLOCK_NODE* _freeNode; // ���ʿ� freeNode�� ����? ó������ �� �� �ִ� ��
	int _capacity = 0; // ���� Ȯ�� �� �� ���� ( �޸�Ǯ ������ ���� ����)
	int _useCount = 0; // useCount ?? ���� ������� ������? 
};


// bPlacementNew�� false�� ���� Ư��ȭ
template <class DATA>
class CObjectPool<DATA, false>
{

	struct st_BLOCK_NODE
	{
		DATA data;
		st_BLOCK_NODE* next = nullptr;
	};

public:

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CObjectPool()
	{
		// Alloc�� ������ / Free�� �ı��� ȣ�� ����
	}

	CObjectPool(int blockNum)
	{
		// Alloc�� ������ / Free �� �ı��� ȣ�� ����

		//TODO: iBlockNum��ŭ Alloc
		if (blockNum == 0)
		{
			// TODO: NOTHING
		}
		else {
			_freeNode = new(st_BLOCK_NODE);
			_freeNode->next = nullptr;
			int i = 0;
			for (; i < blockNum - 1; i++)
			{
				st_BLOCK_NODE* node = new(st_BLOCK_NODE);
				node->next = _freeNode;
				_freeNode = node;
			}

		}
		_capacity += blockNum;
	}

	virtual	~CObjectPool()
	{
		//TODO: �Ҹ��ڿ��� �Ұ�
		// 1. ��ġ
		// 2. Ǯ�ȿ� �����ִ°͸� �Ÿ� ����
		// 3. �� �����ؼ� �޸� ����
	}


	DATA* Alloc(void)
	{
		_useCount++;
		if (_freeNode != nullptr)
		{
			DATA* ret = &_freeNode->data;
			_freeNode = _freeNode->next;
			// ������ΰ� 
			return ret;
		}
		else
		{
			st_BLOCK_NODE* node = new st_BLOCK_NODE;
			//DATA* ret = &(node->data);
			node->next = _freeNode;
			_capacity++;
			return &(node->data);

		}

	}

	void	Free(DATA* pData)
	{
		_useCount--;
		((st_BLOCK_NODE*)pData)->next = _freeNode;
		_freeNode = (st_BLOCK_NODE*)pData;
	}


	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) {
		return _useCount;
	}

	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.
private:
	st_BLOCK_NODE* _freeNode; // ���ʿ� freeNode�� ����? ó������ �� �� �ִ� ��
	int _capacity = 0; // ���� Ȯ�� �� �� ���� ( �޸�Ǯ ������ ���� ����)
	int _useCount = 0; // useCount ?? ���� ������� ������? 
};