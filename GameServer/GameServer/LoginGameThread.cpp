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

void LoginGameThread::HandleLogin(Player * player, CPacket * packet)
{
	// ID와 Password 추출
	TCHAR ID[NICKNAME_LEN];
	TCHAR PassWord[PASS_LEN];
	packet->GetData((char*)ID, NICKNAME_LEN * sizeof(TCHAR));
	packet->GetData((char*)PassWord, PASS_LEN * sizeof(TCHAR));

	// TCHAR를 char로 변환
	char id[NICKNAME_LEN * sizeof(TCHAR)];
	char password[PASS_LEN * sizeof(TCHAR)];
	wcstombs(id, ID, NICKNAME_LEN * sizeof(TCHAR));
	wcstombs(password, PassWord, PASS_LEN * sizeof(TCHAR));

	// 쿼리 문자열 생성
	char query[1024];
	sprintf(query, "SELECT a.AccountNo, p.PlayerID, p.NickName, p.Class, p.Level, p.Exp \
		FROM Account a \
		LEFT JOIN Player p ON a.AccountNo = p.AccountNo \
		WHERE a.ID = '%s' AND a.PassWord = '%s'", id, password);

	// 쿼리 실행
	if (mysql_query(&_conn, query))
	{
		fprintf(stderr, "쿼리 실행 실패: %s\n", mysql_error(&_conn));
		return;
	}

	// 결과 가져오기
	MYSQL_RES* result = mysql_store_result(&_conn);
	if (result == NULL)
	{
		fprintf(stderr, "결과 저장 실패: %s\n", mysql_error(&_conn));
		return;
	}

	int64 AccountNo = -1;
	// 결과 처리
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		AccountNo = atoll(row[0]); // 계정 번호
		PlayerInfo playerInfo;
		playerInfo.PlayerID = atoll(row[1]); // 플레이어 ID
		mbstowcs(playerInfo.NickName, row[2], NICKNAME_LEN);
		playerInfo.Class = static_cast<uint16>(atoi(row[3])); // 클래스
		playerInfo.Level = static_cast<uint16>(atoi(row[4])); // 레벨
		playerInfo.Exp = static_cast<uint32>(atoi(row[5])); // 경험치
		player->playerInfos.push_back(playerInfo);
	}
	CPacket* resPacket = CPacket::Alloc();
	uint8 status = AccountNo != -1 ? 1 : 0;
	MP_SC_LOGIN(resPacket, AccountNo, status);
	SendPacket_Unicast(player->_sessionId, resPacket);
	CPacket::Free(resPacket);
	// 정리
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
	//      TCHAR   PassWord[20]     //사용자 PassWord. null포함
	//  }
	//------------------------------------------------------------
	TCHAR ID[20];
	TCHAR PassWord[20];
	packet->GetData((char*)ID, 20 * sizeof(TCHAR));
	packet->GetData((char*)PassWord, 20 * sizeof(TCHAR));

	// TCHAR를 char로 변환
	char id[40];
	char password[40];
	wcstombs(id, ID, 20 * sizeof(TCHAR));
	wcstombs(password, PassWord, 20 * sizeof(TCHAR));

	// 쿼리 문자열 생성
	char query[1024];
	sprintf(query, "INSERT INTO Account(ID, PassWord) VALUES('%s', '%s')", id, password);

	// 쿼리 실행
	bool signUpSuccess = true;
	if (mysql_query(&_conn, query))
	{
		unsigned int errCode = mysql_errno(&_conn);
		if (errCode == 1062) // Duplicate entry for key 'PRIMARY'
		{
			// 중복 ID로 인한 삽입 실패 처리
			fprintf(stderr, "Duplicate ID: %s\n", id);
			signUpSuccess = false;
		}
		else
		{
			fprintf(stderr, "쿼리 실행 실패: %s\n", mysql_error(&_conn));
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




