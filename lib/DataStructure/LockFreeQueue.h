#pragma once
#include <stdio.h>
#include "Type.h"
#include "LockFreeObjectPool.h"

#define _USE_APPLICATION_BIT 47
template<typename T>
class LockFreeQueue
{
public:
	struct Node
	{
		T _data;
		Node* _next;
	};

	LockFreeQueue()
	{
		_head = new Node;
		_head->_next = NULL;
		_tail = _head;

		_size = 0;
		_demaskValue = 0b1111'1111'1111'1111'1;
		_demaskValue <<= 47;
		_demaskValue = ~_demaskValue;
	}

	~LockFreeQueue()
	{
		T data;
		while (true)
		{
			if (Dequeue(data) == false)
			{
				break;
			}
		}

	}

	int64 Size() {
		return _size;
	}

	void Enqueue(T data)
	{
		// node�� �ּ� �տ� ������� �����ϴ� 17bit ����
		int64 maskValue = InterlockedIncrement64(&_masking);
		maskValue <<= _USE_APPLICATION_BIT;

		Node* newNode = _objectPool.Alloc();
		newNode->_data = data;
		newNode->_next = nullptr;

		int64 masked = (int64)newNode;
		masked |= maskValue;

		Node* tail = nullptr;

		for(;;)
		{
			tail = _tail;
			Node* tailDemasked = (Node*)((int64)tail & _demaskValue);
			Node* tailsNext = tailDemasked->_next;

			if (tailsNext == nullptr)
			{
				if (InterlockedCompareExchange64((int64*)&(tailDemasked->_next), (int64)masked, (int64)nullptr) == (int64)nullptr)
				{
					// _tail�� ���� �ٶ󺸰� �ִ� tail�� ������ _tail�� node�� ����
					InterlockedCompareExchange64((int64*)&_tail, (int64)masked, (int64)tail);
					break;
				}
			}
			else {
				//tail �� tailDemasked->_next�� ����
				InterlockedCompareExchange64((int64*)&_tail, (int64)tailsNext, (int64)tail);
			}
		}

		InterlockedIncrement64(&_size);
	}

	bool Dequeue(T& out)
	{
		Node* head = nullptr;
		Node* next = nullptr;
		Node* tail = nullptr;
		T data{};

		for(;;)
		{
			//**
			tail = _tail;
			head = _head;

			Node* tailDemasked = (Node*)((int64)tail & _demaskValue);
			Node* tailsNext = tailDemasked->_next;

			Node* headDemasked = (Node*)((int64)head & _demaskValue);
			next = headDemasked->_next;
			Node* nextDemasked = (Node*)((int64)next & _demaskValue);

			//head�� tail�� �������
			if (head == tail)
			{
				if (tailsNext == nullptr)
				{
					return false;
				}
				//tail�� next�� null�� �ƴϸ� tail�� tail�� next��
				if (tailsNext != nullptr)
				{
					InterlockedCompareExchange64((int64*)&_tail, (int64)tailsNext, (int64)tail);
				}
			}

			if (nextDemasked != nullptr)
			{
				data = nextDemasked->_data;
			}

			if (InterlockedCompareExchange64((int64*)(&_head), (int64)next, (int64)head) == (int64)head)
			{
				out = data;
				_objectPool.Free(headDemasked);
				break;
			}
		}

		InterlockedDecrement64(&_size);
		return true;
	}

	int64 GetObjectPoolCapacity()
	{
		return _objectPool.GetCapacityCount();
	}

private:
	Node* _head;
	Node* _tail;
	int64 _demaskValue = 0;
	int64 _size = 0;
	int64 _masking = 0;
	LockFreeObjectPool<Node, false> _objectPool;
};