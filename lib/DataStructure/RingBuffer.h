#pragma once
#define DEFAULT_RINGBUFFER_SIZE 10000

#include <stdlib.h>
#include <memory.h>
#include <intrin.h>



// useSize = r - f
//
// freeSize = bufferSize - useSize
class RingBuffer
{
public:
	RingBuffer()
	{
		_realBufferSize = _bufferSize + 1;
		_buffer = (char*)malloc(sizeof(char) * _realBufferSize);
		_front = 0;
		_rear = 0;
	}

	RingBuffer(int bufferSize)
	{
		_realBufferSize = bufferSize + 1;
		_buffer = (char*)malloc(sizeof(char) * _realBufferSize);
		_bufferSize = bufferSize;
		_front = 0;
		_rear = 0;
	}

	~RingBuffer()
	{
		delete _buffer;
	}

	void Resize(int size)
	{
	}

	int	GetBufferSize(void)
	{
		return _bufferSize;
	}
	/////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 용량.
	/////////////////////////////////////////////////////////////////////////
	int	GetUseSize()
	{
		int useSize = _rear - _front;
		if (useSize >= 0)
		{
			return useSize;
		}
		else {
			return _realBufferSize + useSize;
		}
	}

	int	GetFreeSize()
	{
		int useSize = GetUseSize();
		return _bufferSize - useSize;
	}

	//data : 10
	//  0   1   2   3   4   5   6   7   8   9
	// [a] [b] [c] [d] [e] [f] [ ] [ ] [ ] [ ]    상황 1
	// [ ] [ ] [ ] [a] [b] [c] [d] [e] [f] [ ]    상황 2
	// [d] [e] [f] [ ] [ ] [ ] [ ] [a] [b] [c]    상황 3
	// [d] [e] [f] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
	//
	// [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
	// rf
	// [h] [i] [j] [a] [b] [c] [d] [e] [f] [g]
	//              rf
	int	Enqueue(char* chpData, int dataSize)
	{
		int f = _front;
		int freeSize = GetFreeSize(f, _rear);
		if (freeSize < dataSize)
		{
			return 0;
		}
		else {
			if (_rear >= f)
			{
				// rear가 뒤인경우
				int backBufferSize = _realBufferSize - _rear;
				if (backBufferSize >= dataSize)
				{
					//상황 1
					// 뒤에 다 넣어야함
					memcpy(&_buffer[_rear], chpData, dataSize);
					_rear = (_rear + dataSize) % _realBufferSize;
				}
				else {
					//상황 2: 나눠넣어야함
					memcpy(&_buffer[_rear], chpData, backBufferSize);
					memcpy(&_buffer[0], chpData + backBufferSize, dataSize - backBufferSize);
					_rear = (_rear + dataSize) % _realBufferSize;
				}
			}
			else {
				// front가 뒤인경우 : 상황 3
				// 그냥 rear부터 넣으면됨
				memcpy(&_buffer[_rear], chpData, dataSize);
				_rear = (_rear + dataSize) % _realBufferSize;
			}
			return dataSize;
		}
	}

	//data : 10
	//  0   1   2   3   4   5   6   7   8   9
	// [a] [b] [c] [d] [e] [f] [ ] [ ] [ ] [ ]    상황 1
	// [ ] [ ] [ ] [a] [b] [c] [d] [e] [f] [ ]    상황 2
	// [d] [e] [f] [ ] [ ] [ ] [ ] [a] [b] [c]    상황 3
	// [f] [ ] [ ] [ ] [ ] [a] [b] [c] [d] [e]

	// [a] [b] [c] [d] [e] [f] [g] [h] [i] [j]
	// front
	// rear

	// [a] [b] [c] [d] [e] [f] [g] [h] [i] [j]
	//                 fr
	int	Dequeue(char* chpDest, int dataSize)
	{
		int r = _rear;
		int useSize = GetUseSize(_front, r);

		if (useSize < dataSize)
		{
			return 0;
		}
		else {
			if (_front >= r)
			{
				// front가 뒤인경우
				int backBufferSize = _realBufferSize - _front;
				if (backBufferSize >= dataSize)
				{
					// 그냥 뺴면 됨
					memcpy(chpDest, &_buffer[_front], dataSize);
					_front = (_front + dataSize) % _realBufferSize;
				}
				else {
					//나눠서 빼면됨
					memcpy(chpDest, &_buffer[_front], backBufferSize);
					memcpy(chpDest + backBufferSize, &_buffer[0], dataSize - backBufferSize);
					_front = (_front + dataSize) % _realBufferSize;
				}
			}
			else {
				// rear가 뒤인경우
				// 그냥 빼면 됨
				memcpy(chpDest, &_buffer[_front], dataSize);
				_front = (_front + dataSize) % _realBufferSize;
			}

			return dataSize;
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 읽어옴. ReadPos 고정.
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기.
	/////////////////////////////////////////////////////////////////////////

	//  0   1   2   3   4   5   6   7   8   9  10
	// [a] [b] [c] [d] [e] [f] [g] [h] [ ] [l] [k]
	//                              r       f
	// 우려되는게뭐지
	//
	// backBufferSize 를 _realBufferSize - _rear나

	// Dequeue나 Peek인 경우 나눠서봐야하는경우가 일단 _front가 뒤인 경우인데

	/// <summary>
	/// 요청한 dataSize만큼 데이터가 없어도 useSize만큼 데이터를 준다
	/// </summary>
	/// <param name="chpDest"> 데이터 시작 지점</param>
	/// <param name="dataSize"> 요청 데이터 크기</param>
	/// <returns> peek한 데이터 크기 </returns>
	int	Peek(char* chpDest, int dataSize)
	{
		int r = _rear;
		int useSize = GetUseSize(_front, r);
		if (useSize < dataSize)
		{
			return 0;
		}
		else {
			if (_front >= r)
			{
				// front가 뒤인경우
				int backBufferSize = _realBufferSize - _front;
				if (backBufferSize >= dataSize)
				{
					// 그냥 뺴면 됨
					memcpy(chpDest, &_buffer[_front], dataSize);
				}
				else {
					//나눠서 빼면됨
					memcpy(chpDest, &_buffer[_front], backBufferSize);
					memcpy(chpDest + backBufferSize, &_buffer[0], dataSize - backBufferSize);
				}
			}
			else {
				// rear가 뒤인경우
				// 그냥 빼면 됨
				memcpy(chpDest, &_buffer[_front], dataSize);
			}
			return dataSize;
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 모든 데이타 삭제.
	//
	// Parameters: 없음.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////
	void ClearBuffer()
	{
		_rear = 0;
		_front = 0;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	// (끊기지 않은 길이)
	//
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	//
	// Parameters: 없음.
	// Return: (int)사용가능 용량.
	////////////////////////////////////////////////////////////////////////

	//  0   1   2   3   4   5   6   7   8   9  10
	// [ ] [b] [c] [d] [e] [f] [g] [h] [i] [ ] [ ]
	//      f                               r
	// [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
	//      r           f
	// [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
	//      rf
	// [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]

	int	GetDirectEnqueueSize(void)
	{
		// DirectEnqueueSize
	/*	int r = _rear;
		int f = _front;*/
		int f = _front;

		//full인경우
		if ((_rear + 1) % _realBufferSize == f)
		{
			return 0;
		}

		if (_rear >= f)
		{
			// f = 0이면 realBufferSize만큼 줄테니까
			// 이는 원하는 상황이 아니니 bufferSize만큼 준다
			if (f == 0)
			{
				return _realBufferSize - _rear - 1;
			}
			else {
				return _realBufferSize - _rear;
			}

			//int freeSize = GetFreeSize(f, _rear);
			//return _bufferSize -(r - f);
			//return freeSize;
			//return __min(_bufferSize, _realBufferSize - _rear);
		}
		else {
			// r < f 인 경우네
			// use Size가 rear - front니까
			//int freeSize = GetFreeSize(f, r);
			int freeSize = GetFreeSize(f, _rear);
			//return _bufferSize -(r - f);
			return freeSize;
		}
	}

	//  0   1   2   3   4   5   6   7   8   9  10
	// [ ] [b] [c] [d] [e] [f] [g] [h] [i] [ ] [ ]
	//      f                               r
	// [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
	//      r           f
	// [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
	//      rf
	// [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ]
	int	GetDirectDequeueSize(void)
	{
		//int f = _front;
		int r = _rear;
		if (_front > r)
		{
			return _realBufferSize - _front;
		}
		else {
			//비거나 _rear가 뒤인경우
			int useSize = GetUseSize(_front, r);
			return useSize;
			//return r - f;
			/*int useSize = GetUseSize();
			return useSize;*/
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	//
	// Parameters: 없음.
	// Return: (int)이동크기
	/////////////////////////////////////////////////////////////////////////
	int	MoveRear(int size)
	{
		// MoveRear는 언제사용되나
		// 일단 알려주시진 않음

		// TODO:
		// rear위치 사이즈만큼
		_rear = (_rear + size) % _realBufferSize;
		return size;
	}

	int	MoveFront(int size)
	{
		//사용법:
		// send 함수 호출시 한번에 바로 못보낼 수도 있음
		// dequeue 함수 호출하고 한번에 바로 못보내면
		// 다시 enqueue를 해야하는데

		// 이렇게 하지말고
		// peek으로 먼저 데이터 본다음에
		// 보낸만큼만 MoveFront해서 보낸처리를 하자
		/*
			char peekBuffer[1000];
			int ret = sendQueue.Peek(peekBuffer, 1000);
			sendQueue.MoveFront(ret);
		*/

		// TODO :
		// front위치 size만큼 늘리기
		_front = (_front + size) % _realBufferSize;

		return size;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 Front 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* GetFrontPtr(void)
	{
		return _buffer + _front;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 RearPos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* GetRearPtr(void)
	{
		return _buffer + _rear;
	}

	// 버퍼 포인터 얻기 WSARecv, WSASend에서 두개
	char* GetBufferPtr(void)
	{
		return _buffer;
	}

private:
	int GetUseSize(int front, int rear)
	{
		int useSize = rear - front;
		if (useSize >= 0)
		{
			return useSize;
		}
		else {
			return _realBufferSize + useSize;
		}
	}

	int GetFreeSize(int front, int rear)
	{
		int useSize = GetUseSize(front, rear);
		return _bufferSize - useSize;
	}

private:
	char* _buffer;
	int _bufferSize = DEFAULT_RINGBUFFER_SIZE;
	int _realBufferSize;
	int _rear;
	int _front;
};
