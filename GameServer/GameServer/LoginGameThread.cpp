#include "LoginGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include "GameThreadInfo.h"
#include "GameGameThread.h"
#include "Log.h"
#include <process.h>
#include "Packet.h"

LoginGameThread::LoginGameThread(GameServer* gameServer, int threadId) : GameThread(threadId, 100)
{
	_gameServer = gameServer;
	SetGameServer((CNetServer*)gameServer);
}

void LoginGameThread::HandleRecvPacket(int64 sessionId, CPacket* packet)
{
	printf("HandleRecvPacket \n");

	uint16 packetType;
	*packet >> packetType;
	//TODO: 패킷 처리
	Player* player;
	// Disconnect 없을때는 그냥 이렇게 해도 되는데
	//player = _playerMap[sessionId];
	auto playerIt = _playerMap.find(sessionId);
	if(playerIt == _playerMap.end())
	{
		LOG(L"LoginGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, HandleRecvPacket", sessionId);
		CPacket::Free(packet);
		return;
	}
	player = (*playerIt).second;



	switch (packetType)
	{
	case PACKET_CS_GAME_REQ_LOGIN:
	{
		// TODO: 로그인 패킷 갯수 확인
		//PRO_BEGIN(L"HandleSectorMove");
		HandleLogin(player, packet);
		//PRO_END(L"HandleSectorMove");
	}
	break;

	case PACKET_CS_GAME_REQ_FIELD_MOVE:
	{
		HandleFieldMove(player, packet);
	}
	break;

	default:
		__debugbreak();
	}

	CPacket::Free(packet);
}


void LoginGameThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
	printf("OnLeaveThread \n");

	//TODO: 여기서 없으면 끝
	if (disconnect)
	{
		//플레이어 먼저 해제해도 됨 플레이어의 재사용으로 인한 문제는 발생할 수 없음
		// 로그인 쓰레드의 로직은 싱글로처리해도 문제가없기에
		_gameServer->FreePlayer(sessionId);
	}

	int deletedNum = _playerMap.erase(sessionId);
	if (deletedNum == 0)
	{
		// 이미 삭제된 경우
		// 더미 기준에서는 발생하면 안됨
		LOG(L"LoginGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}
}

void LoginGameThread::OnEnterThread(int64 sessionId, void* ptr)
{
	printf("OnEnterThread \n");
	//TODO: map에 추가
	//TODO: 플레이어 생성
	Player* player = _gameServer->AllocPlayer(sessionId);
	if (player == nullptr)
	{
		LOG(L"LoginGameThread", LogLevel::Error, L"AllocPlayer Fail : %lld, OnEnterThread", sessionId);
		return;
	}
	player->_sessionId = sessionId;
	//Player* player = _playerPool.Alloc();
	auto result = _playerMap.insert({ sessionId, player });
	if (!result.second)
	{
		//이거는 두번 발생되면 안되는거고
		__debugbreak();
	}
}



void LoginGameThread::HandleLogin(Player* player, CPacket* packet)
{
	static long playerId = 0;
	long pId = InterlockedIncrement(&playerId);
	player->playerID = pId;


	TCHAR ID[NICKNAME_LEN];
	TCHAR PasssWord[PASS_LEN];
	
	//TODO : id랑 password로 mysql에서 조회
	packet->GetData((char*)ID, NICKNAME_LEN * sizeof(TCHAR));
	packet->GetData((char*)PasssWord, PASS_LEN * sizeof(TCHAR));

	// id랑 password로 mysql에서 조회할거
	// AccountNo, NickName, Level, Exp
	CPacket* resPacket = CPacket::Alloc();
	TCHAR NickName[NICKNAME_LEN];
	swprintf_s(NickName, L"ID%ld", pId);
	//TODO : 조회된 NickName
	wmemcpy(player->NickName, NickName, NICKNAME_LEN);
	int64 AccountNo = 1;
	uint8 Status = true;
	uint16 CharacterLevel = 1;
	uint32 Exp = 50;
	MP_SC_LOGIN(resPacket, AccountNo, Status, CharacterLevel, NickName, Exp);
	SendPacket_Unicast(player->_sessionId, resPacket);
	printf("send login\n");
	CPacket::Free(resPacket);
}

void LoginGameThread::HandleFieldMove(Player* player, CPacket* packet)
{
	uint16 fieldID;
	*packet >> fieldID;
	MoveGameThread(fieldID, player->_sessionId, player);
}



