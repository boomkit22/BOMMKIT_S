#pragma once
#include <new.h>
#include <stdlib.h>
#include "Type.h"
#include <utility>




	template <class DATA, bool bPlacementNew>
	class LockFreeObjectPool;

	// bPlacementNew�� true�� ���� Ư��ȭ
	template <class DATA>
	class LockFreeObjectPool<DATA, true>
	{

		struct st_BLOCK_NODE
		{
			DATA data;
			st_BLOCK_NODE* prev;
		};

	public:

		//////////////////////////////////////////////////////////////////////////
		// ������, �ı���.
		//
		// Parameters:	(int) �ʱ� �� ����.
		//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
		// Return:
		//////////////////////////////////////////////////////////////////////////
		LockFreeObjectPool()
		{
			// Alloc�� ������ / Free�� �ı��� ȣ�� ����
			_demaskValue = 0b11111111'11111111'1;
			_demaskValue <<= 47;
			_demaskValue = ~_demaskValue;

		}

		LockFreeObjectPool(int blockNum)
		{
			// Alloc�� ������ / Free �� �ı��� ȣ�� ����
			_demaskValue = 0b11111111'11111111'1;
			_demaskValue <<= 47;
			_demaskValue = ~_demaskValue;
			//TODO: iBlockNum��ŭ Alloc
			if (blockNum == 0)
			{
				// TODO: NOTHING
			}
			else {
				DATA** data = new DATA * [blockNum];
				for (int i = 0; i < blockNum; i++)
				{
					data[i] = Alloc();
				}

				for (int i = 0; i < blockNum; i++)
				{
					Free(data[i]);
				}
				delete[] data;;
			}
			_capacity = blockNum;
		}

		virtual	~LockFreeObjectPool()
		{
			//TODO: �Ҹ��ڿ��� �Ұ�
			// 1. ��ġ
			// 2. Ǯ�ȿ� �����ִ°͸� �Ÿ� ����
			// 3. �� �����ؼ� �޸� ����
		}



		template <typename... Args>
		DATA* Alloc(Args&&... args)
		{
			InterlockedIncrement64(&_useCount);
			DATA* ret = nullptr;
			st_BLOCK_NODE* newTop = nullptr;
			int64 oldTop = NULL;
			int64 oldTopDemasked = 0;
			int64 newTopMasked = 0;
			do {
				oldTop = _top;

				// ������ �׳� oldTop null�� ������ 
				//�Ѱ� �����ؼ� ����
				if (oldTop == NULL)
				{
					InterlockedIncrement64(&_capacity);
					st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
					newNode->prev = nullptr;
					return new(&(newNode->data)) DATA(std::forward<Args>(args)...);
				}
				oldTopDemasked = (int64)oldTop;
				oldTopDemasked &= _demaskValue;
				ret = &(((st_BLOCK_NODE*)oldTopDemasked)->data);

				newTop = ((st_BLOCK_NODE*)oldTopDemasked)->prev;
				newTopMasked = (int64)newTop;
				if (newTopMasked != 0)
				{
					int64 maskValue = InterlockedIncrement64(&_masking);
					maskValue <<= 47;
					newTopMasked |= maskValue;
				}

				if ((void*)newTopMasked == (void*)oldTop)
				{
					__debugbreak();
				}

			} while (InterlockedCompareExchange64(&_top, newTopMasked, (LONG64)oldTop) != (LONG64)oldTop);

			new(ret) DATA(std::forward<Args>(args)...);
			return ret;
		}

		// push
		void	Free(DATA* pData)
		{
			InterlockedDecrement64(&_useCount);
			int64 maskvalue = InterlockedIncrement64(&_masking);
			maskvalue <<= 47;

			pData->~DATA();
			st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)pData;

			int64 topNode = NULL;
			int64 masked = (int64)newNode;
			masked |= maskvalue;


			do {
				topNode = _top;
				int64 topNodeDemasked = topNode & _demaskValue;

				newNode->prev = (st_BLOCK_NODE*)topNodeDemasked;
				if (masked == topNode)
				{
					__debugbreak();
				}
			} while (InterlockedCompareExchange64(&_top, (LONG64)masked, (LONG64)topNode) != (LONG64)topNode);

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

		}


		// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.
	private:
		int64 _top = NULL; // ���ʿ� freeNode�� ����? ó������ �� �� �ִ� ��
		int64 _capacity = 0; // ���� Ȯ�� �� �� ���� ( �޸�Ǯ ������ ���� ����)
		int64 _useCount = 0; // useCount ?? ���� ������� ������? 
		int64 _masking = 0;
		int64 _demaskValue = 0;
	};


	// bPlacementNew�� false�� ���� Ư��ȭ
	template <class DATA>
	class LockFreeObjectPool<DATA, false>
	{

		struct st_BLOCK_NODE
		{
			DATA data;
			st_BLOCK_NODE* prev;
		};

	public:

		//////////////////////////////////////////////////////////////////////////
		// ������, �ı���.
		//
		// Parameters:	(int) �ʱ� �� ����.
		//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
		// Return:
		//////////////////////////////////////////////////////////////////////////
		LockFreeObjectPool()
		{
			// Alloc�� ������ / Free�� �ı��� ȣ�� ����
			_demaskValue = 0b11111111'11111111'1;
			_demaskValue <<= 47;
			_demaskValue = ~_demaskValue;

		}

		LockFreeObjectPool(int blockNum)
		{
			// Alloc�� ������ / Free �� �ı��� ȣ�� ����
			_demaskValue = 0b11111111'11111111'1;
			_demaskValue <<= 47;
			_demaskValue = ~_demaskValue;
			//TODO: iBlockNum��ŭ Alloc
			if (blockNum == 0)
			{
				// TODO: NOTHING
			}
			else {
				DATA** data = new DATA * [blockNum];
				for (int i = 0; i < blockNum; i++)
				{
					data[i] = Alloc();
				}

				for (int i = 0; i < blockNum; i++)
				{
					Free(data[i]);
				}
				delete[] data;;
			}
			_capacity = blockNum;
		}

		virtual	~LockFreeObjectPool()
		{
			//TODO: �Ҹ��ڿ��� �Ұ�
			// 1. ��ġ
			// 2. Ǯ�ȿ� �����ִ°͸� �Ÿ� ����
			// 3. �� �����ؼ� �޸� ����
		}

		DATA* Alloc(void)
		{
			InterlockedIncrement64(&_useCount);
			DATA* ret = nullptr;

			st_BLOCK_NODE* newTop = nullptr;
			int64 oldTop = NULL;
			int64 oldTopDemasked = 0;
			int64 newTopMasked = 0;
			do {
				oldTop = _top;


				// ������ �׳� oldTop null�� ������ 
				//�Ѱ� �����ؼ� ����
				if (oldTop == NULL)
				{
					InterlockedIncrement64(&_capacity);
					st_BLOCK_NODE* node = new st_BLOCK_NODE;
					node->prev = nullptr;
					return &(node->data);
				}
				oldTopDemasked = (int64)oldTop;
				oldTopDemasked &= _demaskValue;
				ret = &(((st_BLOCK_NODE*)oldTopDemasked)->data);

				newTop = ((st_BLOCK_NODE*)oldTopDemasked)->prev;
				newTopMasked = (int64)newTop;
				if (newTopMasked != 0)
				{
					int64 maskValue = InterlockedIncrement64(&_masking);
					maskValue <<= 47;
					newTopMasked |= maskValue;
				}

				if ((void*)newTopMasked == (void*)oldTop)
				{
					__debugbreak();
				}


			} while (InterlockedCompareExchange64(&_top, newTopMasked, (LONG64)oldTop) != (LONG64)oldTop);

			return ret;
		}


		void	Free(DATA* pData)
		{
			InterlockedDecrement64(&_useCount);
			int64 maskvalue = InterlockedIncrement64(&_masking);
			maskvalue <<= 47;

			st_BLOCK_NODE* newNode = (st_BLOCK_NODE*)pData;

			int64 topNode = NULL;
			int64 masked = (int64)newNode;
			masked |= maskvalue;

			do {
				topNode = _top;
				int64 topNodeDemasked = topNode & _demaskValue;

				newNode->prev = (st_BLOCK_NODE*)topNodeDemasked;
				if (masked == topNode)
				{
					__debugbreak();
				}
			} while (InterlockedCompareExchange64(&_top, (LONG64)masked, (LONG64)topNode) != (LONG64)topNode);
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



	private:
		int64 _top = NULL; // ���ʿ� freeNode�� ����? ó������ �� �� �ִ� ��
		int64 _capacity = 0; // ���� Ȯ�� �� �� ���� ( �޸�Ǯ ������ ��ü ����)
		int64 _useCount = 0; // useCount ���� Ǯ���� �Ҵ�޾Ƽ� �����ִ� ����
		int64 _masking = 0;
		int64 _demaskValue = 0;
	};














