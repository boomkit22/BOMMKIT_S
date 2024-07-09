#define _CRT_SECURE_NO_WARNINGS
#include "LoginGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include "GameThreadInfo.h"
#include "GameGameThread.h"
#include "Log.h"
#include <process.h>
#include "Packet.h"
using namespace std;

LoginGameThread::LoginGameThread(GameServer* gameServer, int threadId) : GameThread(threadId, 100)
{
	_gameServer = gameServer;
	SetGameServer((CNetServer*)gameServer);

	//mysql
	mysql_init(&_conn);
	_connection = mysql_real_connect(&_conn, host, user, password, database, port, NULL, 0);
	if (_connection == NULL) {
		fprintf(stderr, "Mysql connection error  %s\n", mysql_error(&_conn));
		__debugbreak();
	}
}

LoginGameThread::~LoginGameThread()
{
	mysql_close(_connection);
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

	case PACKET_CS_GAME_REQ_SIGN_UP:
	{
		HandleSignUp(player, packet);
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

void LoginGameThread::HandleLogin(Player * player, CPacket * packet)
{
	// ID�� Password ����
	TCHAR ID[NICKNAME_LEN];
	TCHAR PassWord[PASS_LEN];
	packet->GetData((char*)ID, NICKNAME_LEN * sizeof(TCHAR));
	packet->GetData((char*)PassWord, PASS_LEN * sizeof(TCHAR));

	// TCHAR�� char�� ��ȯ
	char id[NICKNAME_LEN * sizeof(TCHAR)];
	char password[PASS_LEN * sizeof(TCHAR)];
	wcstombs(id, ID, NICKNAME_LEN * sizeof(TCHAR));
	wcstombs(password, PassWord, PASS_LEN * sizeof(TCHAR));

	// ���� ���ڿ� ����
	char query[1024];
	sprintf(query, "SELECT a.AccountNo, p.PlayerID, p.NickName, p.Class, p.Level, p.Exp \
		FROM Account a \
		LEFT JOIN Player p ON a.AccountNo = p.AccountNo \
		WHERE a.ID = '%s' AND a.PassWord = '%s'", id, password);

	// ���� ����
	if (mysql_query(&_conn, query))
	{
		fprintf(stderr, "���� ���� ����: %s\n", mysql_error(&_conn));
		return;
	}

	// ��� ��������
	MYSQL_RES* result = mysql_store_result(&_conn);
	if (result == NULL)
	{
		fprintf(stderr, "��� ���� ����: %s\n", mysql_error(&_conn));
		return;
	}

	int64 AccountNo = -1;
	// ��� ó��
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		AccountNo = atoll(row[0]); // ���� ��ȣ
		PlayerInfo playerInfo;
		playerInfo.PlayerID = atoll(row[1]); // �÷��̾� ID
		mbstowcs(playerInfo.NickName, row[2], NICKNAME_LEN);
		playerInfo.Class = static_cast<uint16>(atoi(row[3])); // Ŭ����
		playerInfo.Level = static_cast<uint16>(atoi(row[4])); // ����
		playerInfo.Exp = static_cast<uint32>(atoi(row[5])); // ����ġ
		player->playerInfos.push_back(playerInfo);
	}
	CPacket* resPacket = CPacket::Alloc();
	uint8 status = AccountNo != -1 ? 1 : 0;
	MP_SC_LOGIN(resPacket, AccountNo, status);
	SendPacket_Unicast(player->_sessionId, resPacket);
	CPacket::Free(resPacket);
	// ����
	mysql_free_result(result);
}

void LoginGameThread::HandleFieldMove(Player* player, CPacket* packet)
{
	uint16 fieldID;
	*packet >> fieldID;
	MoveGameThread(fieldID, player->_sessionId, player);
}

void LoginGameThread::HandleSignUp(Player* player, CPacket* packet)
{
	printf("HandleSignUp \n");
	//------------------------------------------------------------
	//  {
	//      WORD    Type
	//      TCHAR   ID[20]
	//      TCHAR   PassWord[20]     //����� PassWord. null����
	//  }
	//------------------------------------------------------------
	TCHAR ID[20];
	TCHAR PassWord[20];
	packet->GetData((char*)ID, 20 * sizeof(TCHAR));
	packet->GetData((char*)PassWord, 20 * sizeof(TCHAR));

	// TCHAR�� char�� ��ȯ
	char id[40];
	char password[40];
	wcstombs(id, ID, 20 * sizeof(TCHAR));
	wcstombs(password, PassWord, 20 * sizeof(TCHAR));

	// ���� ���ڿ� ����
	char query[1024];
	sprintf(query, "INSERT INTO Account(ID, PassWord) VALUES('%s', '%s')", id, password);

	// ���� ����
	bool signUpSuccess = true;
	if (mysql_query(&_conn, query))
	{
		unsigned int errCode = mysql_errno(&_conn);
		if (errCode == 1062) // Duplicate entry for key 'PRIMARY'
		{
			// �ߺ� ID�� ���� ���� ���� ó��
			fprintf(stderr, "Duplicate ID: %s\n", id);
			signUpSuccess = false;
		}
		else
		{
			fprintf(stderr, "���� ���� ����: %s\n", mysql_error(&_conn));
			signUpSuccess = false;
		}
	}

	CPacket* resPacket = CPacket::Alloc();
	uint8 Status = signUpSuccess ? 1 : 0;
	MP_SC_GAME_RES_SIGN_UP(resPacket, Status);
	SendPacket_Unicast(player->_sessionId, resPacket);
	printf("send sign up\n");
	CPacket::Free(resPacket);
}




