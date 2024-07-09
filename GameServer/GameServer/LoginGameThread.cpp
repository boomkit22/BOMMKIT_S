#define _CRT_SECURE_NO_WARNINGS
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
	sprintf(query, "SELECT a.AccountNo, p.PlayerID, p.CharacterLevel, p.NickName, p.Exp FROM Account a LEFT JOIN Player p ON a.AccountNo = p.AccountNo WHERE a.ID = '%s' AND a.PassWord = '%s'", id, password);

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

	// ��� ó��
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		printf("yes");
		//// ����κ��� �� �ʵ� ���� �����ϰ� ������ Ÿ������ ��ȯ
		//int64 AccountNo = atoll(row[0]); // ���� ��ȣ
		//int64 PlayerID = atoll(row[1]); // �÷��̾� ID
		//uint16 CharacterLevel = static_cast<uint16>(atoi(row[2])); // ĳ���� ����
		//char* NickName = row[3]; // �г���
		//uint32 Exp = static_cast<uint32>(atoi(row[4])); // ����ġ

		//// ������ �����͸� ����Ͽ� �ʿ��� �۾� ����
		//// ��: �÷��̾� ��ü�� ������ ����
		//player->AccountNo = AccountNo;
		//player->PlayerID = PlayerID;
		//player->CharacterLevel = CharacterLevel;
		//mbstowcs(player->NickName, NickName, strlen(NickName) + 1); // char*�� TCHAR�� ��ȯ
		//player->Exp = Exp;

		//// �α��� ���� ó�� ����
		//// ��: Ŭ���̾�Ʈ�� �α��� ���� �޽��� ����
		//CPacket* resPacket = CPacket::Alloc();
		//uint8 Status = true; // �α��� ���� ����
		//MP_SC_LOGIN(resPacket, AccountNo, Status, CharacterLevel, player->NickName, Exp);
		//SendPacket_Unicast(player->_sessionId, resPacket);
		//printf("�α��� ���� ����\n");
		//CPacket::Free(resPacket);
	}

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
//	{
//		WORD	Type
// 		TCHAR   ID[20]
//		TCHAR	PassWord[20]     //����� PassWord. null����
//	}
//------------------------------------------------------------
	TCHAR ID[20];
	TCHAR PassWord[20];
	packet->GetData((char*)ID, 20 * sizeof(TCHAR));
	packet->GetData((char*)PassWord, 20 * sizeof(TCHAR));

	bool signUpSuccees = false;
	// TCHAR�� char�� ��ȯ
	char id[40];
	char password[40];
	wcstombs(id, ID, 20 * sizeof(TCHAR));
	wcstombs(password, PassWord, 20 * sizeof(TCHAR));

	// ���� �غ�
	const char* query = "INSERT INTO Account(ID, PassWord) VALUES(?, ?)";

	MYSQL_STMT* stmt = mysql_stmt_init(&_conn);
	if (mysql_stmt_prepare(stmt, query, strlen(query)))
	{
		fprintf(stderr, "mysql_stmt_prepare() failed\n");
		signUpSuccees = false;
	}

	// �Ķ���� ���ε�
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)id;
	bind[0].buffer_length = strlen(id);

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)password;
	bind[1].buffer_length = strlen(password);

	if (mysql_stmt_bind_param(stmt, bind))
	{
		fprintf(stderr, "mysql_stmt_bind_param() failed\n");
		signUpSuccees = false;
	}

	// ���� ����
	if (mysql_stmt_execute(stmt))
	{
		unsigned int errCode = mysql_errno(&_conn);
		if (errCode == 1062) // Duplicate entry for key 'PRIMARY'
		{
			// �ߺ� ID�� ���� ���� ���� ó��
			fprintf(stderr, "Duplicate ID: %s\n", id);
			signUpSuccees = false;
		}
		else
		{
			fprintf(stderr, "mysql_stmt_execute() failed: %s\n", mysql_error(&_conn));
			signUpSuccees = false;
		}
	}
	else
	{
		signUpSuccees = true;
	}

	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);

	CPacket* resPacket = CPacket::Alloc();
	uint8 Status = signUpSuccees;
	MP_SC_GAME_RES_SIGN_UP(resPacket, Status);
	SendPacket_Unicast(player->_sessionId, resPacket);
	printf("send sign up\n");
	CPacket::Free(resPacket);
}



