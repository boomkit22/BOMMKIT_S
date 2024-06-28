#include "EchoGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include <process.h>
#include "GameThreadInfo.h"
#include "Log.h"
#include "Packet.h"
using namespace std;


EchoGameThread::EchoGameThread(GameServer* gameServer,int threadId) : GameThread(threadId, 1)
{
	_gameServer = gameServer;
	SetGameServer((CNetServer*)gameServer);
}

void EchoGameThread::HandleRecvPacket(int64 sessionId, CPacket * packet)
{
	Player* player = nullptr;
	auto it = _playerMap.find(sessionId);
	if (it == _playerMap.end())
	{
		LOG(L"EchoGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, HandleRecvPacket", sessionId);
		return;
	}
	player = it->second;

	uint16 packetType;
	*packet >> packetType;

	switch (packetType)
	{

	case PACKET_CS_GAME_REQ_CHARACTER_MOVE:
	{
		LOG(L"Develop", LogLevel::Debug, L"Login Recv");

		// 로그인  메시지
		PRO_BEGIN(L"HandleLogin");
		HandleCharacterMove(player, packet);
		PRO_END(L"HandleLogin");
	}
	break;

	default:
		LOG(L"Packet", LogLevel::Error, L"Packet Type Not Exist");
		__debugbreak();//TODO: 악의적인 유저가 이상한 패킷을 보냈다-> 세션 끊는다
	}

	CPacket::Free(packet);
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
	printf("send field move\n");
	CPacket::Free(packet);

	// 내 캐릭터 소환 패킷 보내고
	int spawnX = rand() % 400;
	int spawnY = rand() % 400;
	CPacket* spawnCharacterPacket = CPacket::Alloc();
	FVector spawnLocation{ spawnX, spawnY, 100 };
	SpawnMyCharacterInfo spawnMyCharacteRInfo;
	spawnMyCharacteRInfo.SpawnLocation = spawnLocation;
	wmemcpy(spawnMyCharacteRInfo.NickName, p->NickName, NICKNAME_LEN);
	spawnMyCharacteRInfo.Level = p->Level;
	spawnMyCharacteRInfo.PlayerID = p->playerID;

	p->Postion = spawnLocation;

	MP_SC_SPAWN_MY_CHARACTER(spawnCharacterPacket, spawnMyCharacteRInfo);
	SendPacket_Unicast(p->_sessionId, spawnCharacterPacket);
	printf("send spawn my character\n");
	CPacket::Free(spawnCharacterPacket);

	//TODO: 다른 컈릭터들에게 이 캐릭터 소환 패킷 보내고
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		if (it->first == sessionId)
			continue;
		Player* other = it->second;

		CPacket* spawnOtherCharacterPacket = CPacket::Alloc();
		SpawnOtherCharacterInfo spawnOtherCharacterInfo;
		spawnOtherCharacterInfo.SpawnLocation = p->Postion;
		printf("to other Spawn Location : %f, %f, %f\n", p->Postion.X, p->Postion.Y, p->Postion.Z);

		wmemcpy(spawnOtherCharacterInfo.NickName, p->NickName, NICKNAME_LEN);
		//spawnOtherCharacterInfo.NickName = p->NickName;
		spawnOtherCharacterInfo.Level = p->Level;
		spawnOtherCharacterInfo.PlayerID = p->playerID;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, spawnOtherCharacterInfo);
		SendPacket_Unicast(other->_sessionId, spawnOtherCharacterPacket);
		printf("to other send spawn other character\n");
		CPacket::Free(spawnOtherCharacterPacket);
	}

	//TODO: 이 캐릭터에게 이미 존재하고 있던 다른 캐릭터들 패킷 보내고
	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		if (it->first == sessionId)
			continue;
		Player* other = it->second;

		CPacket* spawnOtherCharacterPacket = CPacket::Alloc();
		SpawnOtherCharacterInfo spawnOtherCharacterInfo;
		spawnOtherCharacterInfo.SpawnLocation = other->Postion;
		printf("to me Spawn Location : %f, %f, %f\n", other->Postion.X, other->Postion.Y, other->Postion.Z);

		wmemcpy(spawnOtherCharacterInfo.NickName, other->NickName, NICKNAME_LEN);
		//spawnOtherCharacterInfo.NickName = p->NickName;
		spawnOtherCharacterInfo.Level = other->Level;
		spawnOtherCharacterInfo.PlayerID = other->playerID;
		MP_SC_SPAWN_OTHER_CHARACTER(spawnOtherCharacterPacket, spawnOtherCharacterInfo);
		SendPacket_Unicast(p->_sessionId, spawnOtherCharacterPacket);
		printf("to me send spawn other character\n");
		CPacket::Free(spawnOtherCharacterPacket);
	}


	//TODO: 몬스터들 소환 패킷 보내고 
}

void EchoGameThread::HandleCharacterMove(Player* p, CPacket* packet)
{
	//TODO: 모든 유저에게 패킷 브로드캐스팅
	int64 characterNo = p->playerID;
	FVector destination;
	*packet >> destination;

	CPacket* movePacket = CPacket::Alloc();
	MP_SC_GAME_RES_CHARACTER_MOVE(movePacket, characterNo, destination);

	for (auto it = _playerMap.begin(); it != _playerMap.end(); it++)
	{
		SendPacket_Unicast(it->first, movePacket);
	}
}

