#include "GameThread.h"
#include "Log.h"
#include "SerializeBuffer.h"
#include <process.h>
#include "CNetServer.h"


std::map<int, class GameThread*> _gameThreadInfoMap;
SRWLOCK _gameThreadInfoMapLock = SRWLOCK_INIT;

GameThread::GameThread(int threadId, int msPerFram) : _msPerFrame(msPerFram)
{
	InitializeSRWLock(&_gameThreadInfoMapLock);
	_originalMsPerFrame = msPerFram;

	AcquireSRWLockExclusive(&_gameThreadInfoMapLock);
	_gameThreadInfoMap.insert({ threadId, this });
	ReleaseSRWLockExclusive(&_gameThreadInfoMapLock);
	
	_gameThreadID = threadId;

	_hAsyncJobThreadEvent = 
		CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void GameThread::Start()
{
	int errorCode;

	_hUpdateThread = (HANDLE)_beginthreadex(nullptr, 0, UpdateThreadStatic, this, 0, nullptr);
	if (_hUpdateThread == NULL)
	{
		errorCode = WSAGetLastError();
		__debugbreak();
	}
	_runningThread++;

	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThreadStatic, this, 0, NULL);
	if (_hMonitorThread == NULL)
	{
		errorCode = WSAGetLastError();
		__debugbreak();
	}
	_runningThread++;

	_hAsyncJobThread = (HANDLE)_beginthreadex(NULL, 0, AsyncJobThreadStatic, this, 0, NULL);
	if (_hAsyncJobThread == NULL)
	{
		errorCode = WSAGetLastError();
		__debugbreak();
	}
	_runningThread++;
}

unsigned __stdcall GameThread::UpdateThread()
{
	DWORD time = timeGetTime();
	DWORD lastLogicProcess = time;
	int delay = 0;
	
	
	
	while (_running)
	{
		DWORD currentTime = timeGetTime();
		delay = currentTime - lastLogicProcess;

		// deltaTime을 밀리초 단위로 계산
		int deltaTime = delay; // 밀리초 단위

		NetworkRun();
		GameRun(deltaTime / 1000.f); // deltaTime을 초로
		_updateTps++;

		lastLogicProcess = currentTime;

		// 프레임 맞추기
		DWORD frameTime = timeGetTime() - currentTime;
		if (frameTime < _msPerFrame)
		{
			Sleep(_msPerFrame - frameTime);
		}
	}

	_runningThread--;
	return 0;
}

unsigned int __stdcall GameThread::MonitorThread()
{
	int tpsIndex = 0;
	while (_running)
	{
		Sleep(1000);
		_packetTps[tpsIndex % TPS_ARR_NUM] = _updateTps;
		_updateTps = 0;
		tpsIndex++;
	}

	_runningThread--;
	return 0;
}

unsigned int __stdcall GameThread::AsyncJobThread()
{
	while (_running)
	{
		WaitForSingleObject(_hAsyncJobThreadEvent, INFINITE);

		for (;;)
		{
			std::pair<Session*, std::function<void()>> asyncJob;
			bool dequeueSucceed = _asyncJobQueue.Dequeue(asyncJob);
			if (!dequeueSucceed)
			{
				break;
			}
			Session* s = asyncJob.first;
			//호출해주고
			asyncJob.second();
			CPacket* packet = CPacket::Alloc();
			HandleAsyncJobFinish(s->_sessionId);
			CPacket::Free(packet);
			InterlockedDecrement(&s->_bProcessingAsyncJobNum);
			_netServer->PutBackSession(s);

		}
	}

	return 0;
}


void GameThread::NetworkRun()
{
	if (_enterQueue.Size() != 0)
	{
		ProcessEnter();
	}
	if (_leaveQueue.Size() != 0)
	{
		ProcessLeave();
	}

	int sessionNum = _sessionArr.size();
	if (sessionNum == 0)
	{
		_msPerFrame = 1000;
	}
	else
	{
		_msPerFrame = _originalMsPerFrame;
	}


	for (int i = 0; i < sessionNum; i++)
	{
		int64 sessionId = _sessionArr[i];
		Session* s = _netServer->GetSession(sessionId);
		if (s == nullptr)
		{
			continue;
		}
		if(s->_bProcessingAsyncJobNum > 0)
		{
			_netServer->PutBackSession(s);
			continue;
		}

		if (s->_packetQueue.Size() == 0)
		{
			_netServer->PutBackSession(s);
			continue;
		}

		std::vector<CPacket*> packets;
		packets.reserve(500);

		while (true)
		{
			CPacket* packet = nullptr;

			bool dequeueSucceed = s->_packetQueue.Dequeue(packet);
			if (!dequeueSucceed)
			{
				break;
			}

			packets.push_back(packet);

		}

		HandleRecvPacket(sessionId, packets[0]);
		_netServer->PutBackSession(s);
	}
}




// 이미 끊어졌으면 return fale
bool GameThread::EnterSession(int64 sessionId, void* ptr)
{
	Session* session = _netServer->GetSession(sessionId);
	if (session == nullptr)
	{
		_netServer->PutBackSession(session);
		return false;
	}
	session->_gameThread = this;
	_netServer->PutBackSession(session);

	_enterQueue.Enqueue({ sessionId, ptr });
	//SetEvent(_hUpdateEvent);
	return true;
}

void GameThread::LeaveSession(int64 sessionId, bool disconnect)
{
	_leaveQueue.Enqueue({ sessionId, disconnect });
}

void GameThread::ProcessEnter()
{
	EnterSessionInfo enterSessionInfo;
	while (_running)
	{
		bool dequeueSucceed = _enterQueue.Dequeue(enterSessionInfo);
		if (!dequeueSucceed)
		{
			break;
		}
		int64 sessionId = enterSessionInfo.sessionId;
		//Session* session = _netServer->GetSession(sessionId);
		//if (session == nullptr)
		//{
		//	continue;
		//}
		//_netServer->PutBackSession(session);

		OnEnterThread(sessionId, enterSessionInfo.ptr);
		auto it = std::find(_sessionArr.begin(), _sessionArr.end(), sessionId);
		if (it != _sessionArr.end())
		{
			LOG(L"GameThread", LogLevel::Error, L"AlreadyExist : %lld, ProcessEnter", sessionId);
			continue;
		}
		_sessionArr.push_back(sessionId);
	}
}

void GameThread::ProcessLeave()
{
	//int64 sessionId;

	//두개가 들어올수도있잖아
	// 컨텐츠에서 MoveThread랑
	// 라이브러리에서 Disconnect랑

	LeaveSessionInfo leaveSessionInfo;
	while (_running)
	{
		bool dequeueSucceed = _leaveQueue.Dequeue(leaveSessionInfo);
		if (!dequeueSucceed)
		{
			break;
		}
		
		int64 sessionId = leaveSessionInfo.sessionId;
		/*Session* session = _netServer->GetSession(sessionId);
		if (session == nullptr)
		{
			continue;
		}

		_netServer->PutBackSession(session);*/
		auto it = std::find(_sessionArr.begin(), _sessionArr.end(), sessionId);
		if (it == _sessionArr.end())
		{
			LOG(L"GameThread", LogLevel::Error, L"Cannot find sessionId : %lld, ProcessLeave", sessionId);
			continue;
		}

		OnLeaveThread(sessionId, leaveSessionInfo.disconnect);
		_sessionArr.erase(it);
	}
}


bool GameThread::RequestAsyncJob(int64 sessionId, std::function<void()> job)
{
	Session* s = _netServer->GetSession(sessionId);
	if (s == nullptr)
	{
		return false;
	}
	InterlockedIncrement(&s->_bProcessingAsyncJobNum);
	//s->_bProcessingAsyncJobNum++;
	_asyncJobQueue.Enqueue({ s, job });
	SetEvent(_hAsyncJobThreadEvent);
	//_netServer->PutBackSession(s);

	return true;
}




void GameThread::SendPackets_Unicast(int64 sessionId, std::vector<CPacket*>& packets)
{
	_netServer->SendPackets(sessionId, packets);
}


void GameThread::SendPacket_Unicast(int64 sessionId, CPacket* packet)
{
	//TODO: inline으로 변경되는지 확인하고
	_netServer->SendPacket(sessionId, packet);
}

bool GameThread::MoveGameThread(int gameThreadId, int64 sessionId, void* ptr)
{
	//여기서 찾아서 없으면
	GameThread* gameThread;
	AcquireSRWLockShared(&_gameThreadInfoMapLock);
	auto it = _gameThreadInfoMap.find(gameThreadId);
	if (it == _gameThreadInfoMap.end())
	{
		ReleaseSRWLockShared(&_gameThreadInfoMapLock);
		return false;
	}
	gameThread = (*it).second;
	ReleaseSRWLockShared(&_gameThreadInfoMapLock);

	LeaveSession(sessionId, false);
	gameThread->EnterSession(sessionId, ptr);
	return true;
}



//TODO: Stop
void GameThread::Stop()
{
	AcquireSRWLockExclusive(&_gameThreadInfoMapLock);
	_gameThreadInfoMap.erase(_gameThreadID);
	ReleaseSRWLockExclusive(&_gameThreadInfoMapLock);

	_running = false;
}


bool GameThread::CheckDead()
{
	if (WaitForSingleObject(_hUpdateThread, 0) == WAIT_TIMEOUT)
	{
		return false;
	}

	if (WaitForSingleObject(_hMonitorThread, 0) == WAIT_TIMEOUT)
	{
		return false;
	}
	
	return true;
}


void GameThread::Kill()
{
	if (WaitForSingleObject(_hUpdateThread, 0) == WAIT_TIMEOUT)
	{
		TerminateThread(_hUpdateThread, 0);
	}

	if (WaitForSingleObject(_hMonitorThread, 0) == WAIT_TIMEOUT)
	{
		TerminateThread(_hMonitorThread, 0);
	}

	if (WaitForSingleObject(_hAsyncJobThread, 0) == WAIT_TIMEOUT)
	{
		TerminateThread(_hAsyncJobThread, 0);
	}
			
}

int64 GameThread::GetSessionNum()
{
	return _sessionArr.size() + _enterQueue.Size(); +_leaveQueue.Size();
}

std::vector<int64> GameThread::GetSessions()
{
	std::vector<int64> sessions;
	for(int64 s: _sessionArr)
	{
		sessions.push_back(s);
	}

	EnterSessionInfo enterSessionInfo;
	while (_enterQueue.Dequeue(enterSessionInfo))
	{
		sessions.push_back(enterSessionInfo.sessionId);
	}

	LeaveSessionInfo leaveSessionInfo;
	while (_leaveQueue.Dequeue(leaveSessionInfo))
	{
		sessions.push_back(leaveSessionInfo.sessionId);
	}

	return sessions;
}