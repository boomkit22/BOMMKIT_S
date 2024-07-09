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
	sprintf(query, "SELECT a.AccountNo, p.PlayerID, p.CharacterLevel, p.NickName, p.Exp FROM Account a LEFT JOIN Player p ON a.AccountNo = p.AccountNo WHERE a.ID = '%s' AND a.PassWord = '%s'", id, password);

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

	// 결과 처리
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		printf("yes");
		//// 결과로부터 각 필드 값을 추출하고 적절한 타입으로 변환
		//int64 AccountNo = atoll(row[0]); // 계정 번호
		//int64 PlayerID = atoll(row[1]); // 플레이어 ID
		//uint16 CharacterLevel = static_cast<uint16>(atoi(row[2])); // 캐릭터 레벨
		//char* NickName = row[3]; // 닉네임
		//uint32 Exp = static_cast<uint32>(atoi(row[4])); // 경험치

		//// 추출한 데이터를 사용하여 필요한 작업 수행
		//// 예: 플레이어 객체에 데이터 설정
		//player->AccountNo = AccountNo;
		//player->PlayerID = PlayerID;
		//player->CharacterLevel = CharacterLevel;
		//mbstowcs(player->NickName, NickName, strlen(NickName) + 1); // char*를 TCHAR로 변환
		//player->Exp = Exp;

		//// 로그인 성공 처리 로직
		//// 예: 클라이언트에 로그인 성공 메시지 전송
		//CPacket* resPacket = CPacket::Alloc();
		//uint8 Status = true; // 로그인 성공 상태
		//MP_SC_LOGIN(resPacket, AccountNo, Status, CharacterLevel, player->NickName, Exp);
		//SendPacket_Unicast(player->_sessionId, resPacket);
		//printf("로그인 성공 전송\n");
		//CPacket::Free(resPacket);
	}

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
//	{
//		WORD	Type
// 		TCHAR   ID[20]
//		TCHAR	PassWord[20]     //사용자 PassWord. null포함
//	}
//------------------------------------------------------------
	TCHAR ID[20];
	TCHAR PassWord[20];
	packet->GetData((char*)ID, 20 * sizeof(TCHAR));
	packet->GetData((char*)PassWord, 20 * sizeof(TCHAR));

	bool signUpSuccees = false;
	// TCHAR를 char로 변환
	char id[40];
	char password[40];
	wcstombs(id, ID, 20 * sizeof(TCHAR));
	wcstombs(password, PassWord, 20 * sizeof(TCHAR));

	// 쿼리 준비
	const char* query = "INSERT INTO Account(ID, PassWord) VALUES(?, ?)";

	MYSQL_STMT* stmt = mysql_stmt_init(&_conn);
	if (mysql_stmt_prepare(stmt, query, strlen(query)))
	{
		fprintf(stderr, "mysql_stmt_prepare() failed\n");
		signUpSuccees = false;
	}

	// 파라미터 바인딩
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

	// 쿼리 실행
	if (mysql_stmt_execute(stmt))
	{
		unsigned int errCode = mysql_errno(&_conn);
		if (errCode == 1062) // Duplicate entry for key 'PRIMARY'
		{
			// 중복 ID로 인한 삽입 실패 처리
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



