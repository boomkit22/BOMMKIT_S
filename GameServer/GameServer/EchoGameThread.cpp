#include "EchoGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include <process.h>
#include "GameThreadInfo.h"
#include "Log.h"
using namespace std;


EchoGameThread::EchoGameThread(GameServer* gameServer,int threadId) : GameThread(threadId, 1)
{
	_gameServer = gameServer;
	SetGameServer((CNetServer*)gameServer);
}

void EchoGameThread::HandleRecvPacket(int64 sessionId, CPacket * packet)
{
	PRO_BEGIN(L"HandleRecvPacket");
	Player* player = nullptr;
	auto playerIt = _playerMap.find(sessionId);
	if (playerIt == _playerMap.end())
	{
		LOG(L"EchoGameThread", LogLevel::Error, L"Cannot find sessionId, HandleRecvPacket");
		CPacket::Free(packet);
		return;
	}
	//player = (*playerIt).second;
	CPacket* resPacket = CPacket::Alloc();
	MP_SC_ECHO(resPacket, packet);
	SendPacket_Unicast(sessionId, resPacket);
	CPacket::Free(resPacket);
	CPacket::Free(packet);
	PRO_END(L"HandleRecvPacket");
}


void EchoGameThread::HandleRecvPacket(int64 sessionId, std::vector<CPacket*>& packets)
{
	PRO_BEGIN(L"HandleRecvPackets");
	Player* player = nullptr;
	auto playerIt = _playerMap.find(sessionId);
	if (playerIt == _playerMap.end())
	{
		LOG(L"EchoGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, HandleRecvPacket vec", sessionId);
		for (CPacket* p : packets)
		{
			CPacket::Free(p);
		}
		return;
	}

	//player = (*playerIt).second;

	vector<CPacket*> sendPacket;
	sendPacket.reserve(500);

	int packetSize = packets.size();
	for (int i = 0; i < packetSize; i++)
	{
		CPacket*& packet = packets[i];
		CPacket* resPacket = CPacket::Alloc();
		MP_SC_ECHO(resPacket, packet);
		CPacket::Free(packet);
		sendPacket.push_back(resPacket);
	}

	SendPackets_Unicast(sessionId, sendPacket);
	for (int i = 0; i < packetSize; i++)
	{
		CPacket::Free(sendPacket[i]);
	}
	PRO_END(L"HandleRecvPackets");
}


void EchoGameThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
	if (disconnect)
	{
		_gameServer->FreePlayer(sessionId);
	}
	else
	{
		LOG(L"EchoGameThread", LogLevel::Error, L"no disconnect : %lld, OnLeaveThread", sessionId);
	}

	int deletedNum = _playerMap.erase(sessionId);
	if (deletedNum == 0)
	{
		// 이미 삭제된 경우
		// 더미 기준에서는 발생하면 안됨
		LOG(L"EchoGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}
}


void EchoGameThread::OnEnterThread(int64 sessionId, void* ptr)
{
	//TODO: map에 추가
	//TODO: 플레이어 생성
	Player* p = (Player*)ptr;
	auto result = _playerMap.insert({ sessionId, p });
	if (!result.second)
	{
		__debugbreak();
	}

	// 필드 이동 응답 보내고
	CPacket* packet = CPacket::Alloc();
	uint8 status = true;
	MP_SC_FIELD_MOVE(packet, status);
	//TODO: send
	SendPacket_Unicast(p->_sessionId, packet);
	CPacket::Free(packet);

	// 내 캐릭터 소환 패킷 보내고
	CPacket* spawnCharacterPacket = CPacket::Alloc();
	FVector spawnLocation{ 0,0,0 };
	MP_SC_SPAWN_MY_CHARACTER(spawnCharacterPacket, spawnLocation);
	SendPacket_Unicast(p->_sessionId, spawnCharacterPacket);
	CPacket::Free(spawnCharacterPacket);

	//TODO: 다른 컈릭터들 소환 패킷 보내고



	//TODO: 몬스터들 소환 패킷 보내고 
}


