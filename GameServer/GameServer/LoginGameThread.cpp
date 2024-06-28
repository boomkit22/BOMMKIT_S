#include "LoginGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include "GameThreadInfo.h"
#include "EchoGameThread.h"
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
	//TODO: ��Ŷ ó��
	Player* player;
	// Disconnect �������� �׳� �̷��� �ص� �Ǵµ�
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
		// TODO: �α��� ��Ŷ ���� Ȯ��
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

	//TODO: ���⼭ ������ ��
	if (disconnect)
	{
		//�÷��̾� ���� �����ص� �� �÷��̾��� �������� ���� ������ �߻��� �� ����
		// �α��� �������� ������ �̱۷�ó���ص� ���������⿡
		_gameServer->FreePlayer(sessionId);
	}

	int deletedNum = _playerMap.erase(sessionId);
	if (deletedNum == 0)
	{
		// �̹� ������ ���
		// ���� ���ؿ����� �߻��ϸ� �ȵ�
		LOG(L"LoginGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
	}
}

void LoginGameThread::OnEnterThread(int64 sessionId, void* ptr)
{
	printf("OnEnterThread \n");
	//TODO: map�� �߰�
	//TODO: �÷��̾� ����
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
		//�̰Ŵ� �ι� �߻��Ǹ� �ȵǴ°Ű�
		__debugbreak();
	}
}



void LoginGameThread::HandleLogin(Player* player, CPacket* packet)
{
	static long playerId = 0;
	long pId = InterlockedIncrement(&playerId);

	//TODO: EchoGroup���� �ű��
	// EchoGroup���� ResMessage������

	int64 accountNo;
	*packet >> accountNo;
	//TODO:
	//mysql���� ��ȸ
	CPacket* resPacket = CPacket::Alloc();
	ResGameLoginInfo resLoginInfo;
	resLoginInfo.Status = true;
	resLoginInfo.AccountNo = accountNo;
	resLoginInfo.Level = 1;

	TCHAR characterID[NICKNAME_LEN];
	swprintf_s(characterID, L"ID%ld", pId);
	wmemcpy(player->NickName, characterID, NICKNAME_LEN);
	wmemcpy(resLoginInfo.NickName, characterID, NICKNAME_LEN);
	//resLoginInfo.NickName= characterID;
	//uint8 status = true;
	uint16 characterLevel = 1;
	player->Level = characterLevel;
	player->playerID = playerId;

	MP_SC_LOGIN(resPacket, resLoginInfo);
	SendPacket_Unicast(player->_sessionId, resPacket);
	printf("send login\n");
	CPacket::Free(resPacket);
	//if (true) // ���� ����
	//{
	//	player->accountNo = accountNo;
	//	MoveGameThread(ECHO_THREAD, player->_sessionId, player);
	//} 
	//else
	//{
	//	// ���� ����
	//	// Disconnect
	//}
}

void LoginGameThread::HandleFieldMove(Player* player, CPacket* packet)
{
	uint16 fieldID;
	*packet >> fieldID;

	MoveGameThread(fieldID, player->_sessionId, player);
}



