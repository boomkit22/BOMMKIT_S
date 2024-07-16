#define _CRT_SECURE_NO_WARNINGS
#include "LoginGameThread.h"
#include "SerializeBuffer.h"
#include "Profiler.h"
#include "GuardianFieldThread.h"
#include "Log.h"
#include <process.h>
#include "Packet.h"
#include "PacketMaker.h"
#include "Player.h"
#include "GameServer.h"

using namespace std;

LoginGameThread::LoginGameThread(GameServer* gameServer, int threadId) : BasePacketHandleThread(gameServer, threadId)
{
	//mysql
	mysql_init(&_conn);
	_connection = mysql_real_connect(&_conn, host, user, password, database, port, NULL, 0);
	if (_connection == NULL) {
		fprintf(stderr, "Mysql connection error  %s\n", mysql_error(&_conn));
		__debugbreak();
	}

	// last_player_id 테이블에서 마지막 PlayerID 가져오기
	const char* query = "SELECT PlayerID FROM last_player_id LIMIT 1";
	if (mysql_query(_connection, query))
	{
		fprintf(stderr, "querry error: %s\n", mysql_error(_connection));
	}
	else 
	{
		MYSQL_RES* result = mysql_store_result(_connection);
		if (result)
		{
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row && row[0])
			{
				lastPlayerId = atoll(row[0]); // 문자열을 long long 타입으로 변환
			}
			mysql_free_result(result);
		}
	}

	RegisterPacketHandler(PACKET_CS_GAME_REQ_LOGIN, [this](Player* p, CPacket* packet) { HandleLogin(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_FIELD_MOVE, [this](Player* p, CPacket* packet) { HandleFieldMove(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_SIGN_UP, [this](Player* p, CPacket* packet) { HandleSignUp(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_PLAYER_LIST, [this](Player* p, CPacket* packet) { HandleRequestPlayerList(p, packet); }); 
	RegisterPacketHandler(PACKET_CS_GAME_REQ_SELECT_PLAYER, [this](Player* p, CPacket* packet) { HandleSelectPlayer(p, packet); });
	RegisterPacketHandler(PACKET_CS_GAME_REQ_CREATE_PLAYER, [this](Player* p, CPacket* packet) { HandleCreatePlayer(p, packet); });
}

LoginGameThread::~LoginGameThread()
{
	mysql_close(_connection);
}

void LoginGameThread::OnEnterThread(int64 sessionId, void* ptr)
{
	printf("OnEnterThread \n");
	//TODO: map에 추가
	//TODO: 플레이어 생성
	Player* player = AllocPlayer(sessionId);
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

void LoginGameThread::GameRun(float deltaTime)
{

}

void LoginGameThread::OnLeaveThread(int64 sessionId, bool disconnect)
{
	printf("OnLeaveThread \n");

	//TODO: 여기서 없으면 끝
	if (disconnect)
	{
		//플레이어 먼저 해제해도 됨 플레이어의 재사용으로 인한 문제는 발생할 수 없음
		// 로그인 쓰레드의 로직은 싱글로처리해도 문제가없기에
		FreePlayer(sessionId);
	}

	int deletedNum = _playerMap.erase(sessionId);
	if (deletedNum == 0)
	{
		// 이미 삭제된 경우
		// 더미 기준에서는 발생하면 안됨
		LOG(L"LoginGameThread", LogLevel::Error, L"Cannot find sessionId : %lld, OnLeaveThread", sessionId);
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
		if (row[1] != NULL)
		{
			PlayerInfo playerInfo;
			playerInfo.PlayerID = atoll(row[1]); // 플레이어 ID
			mbstowcs(playerInfo.NickName, row[2], NICKNAME_LEN);
			playerInfo.Class = static_cast<uint16>(atoi(row[3])); // 클래스
			playerInfo.Level = static_cast<uint16>(atoi(row[4])); // 레벨
			playerInfo.Exp = static_cast<uint32>(atoi(row[5])); // 경험치
			player->playerInfos.push_back(playerInfo);
		}

	}
	CPacket* resPacket = CPacket::Alloc();
	uint8 status = false;
	if (AccountNo != -1)
	{
		status = true;
		player->accountNo = AccountNo;
	}

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
	CPacket::Free(resPacket);

	printf("HandleSignUp \n");
}

void LoginGameThread::HandleRequestPlayerList(Player* player, CPacket* packet)
{
	//TODO : player vector에 있는거 보내주기
	CPacket* resPacket = CPacket::Alloc();
	MP_SC_PLAYER_LIST(resPacket, player->playerInfos);
	SendPacket_Unicast(player->_sessionId, resPacket);
	CPacket::Free(resPacket);

	printf("HandleRequestPlayerList \n");
}

void LoginGameThread::HandleSelectPlayer(Player* player, CPacket* packet)
{
	// 여기서 예외처리 할게 뭐가있을까
	int64 playerID;
	*packet >> playerID;
	//TODO : 플레이어 선택
	for (auto playerInfo : player->playerInfos)
	{
		if (playerInfo.PlayerID == playerID)
		{
			player->playerInfo = playerInfo;
			break;
		}
	}

	uint8 Status = true;
	CPacket* resPacket = CPacket::Alloc();
	MP_SC_SELECT_PLAYER(resPacket, Status);
	SendPacket_Unicast(player->_sessionId, resPacket);
	CPacket::Free(resPacket);
	printf("HandleSelectPlayer \n");
}



/*
CREATE TABLE Player (
	PlayerID BIGINT NOT NULL AUTO_INCREMENT,
	AccountNo BIGINT NOT NULL,
	NickName CHAR(40) NOT NULL,
	Class SMALLINT unsigned NOT NULL,
	Level SMALLINT UNSIGNED NOT NULL DEFAULT 1,
	Exp INT UNSIGNED NOT NULL DEFAULT 0,
	PRIMARY KEY (PlayerID),
	FOREIGN KEY (AccountNo) REFERENCES Account(AccountNo)
);
*/
void LoginGameThread::HandleCreatePlayer(Player* player, CPacket* packet)
{
		//TODO : 캐릭터 생성
	// 캐릭터 생성
	//------------------------------------------------------------
	//  {
	//      WORD    Type
	//      TCHAR   NickName[NICKNAME_LEN]
	//      uint16  Class
	//  }
	//------------------------------------------------------------
	TCHAR NickName[NICKNAME_LEN];
	uint16 Class;
	*packet >> Class;
	packet->GetData((char*)NickName, NICKNAME_LEN * sizeof(TCHAR));

	// TCHAR를 char로 변환
	char nickName[NICKNAME_LEN * sizeof(TCHAR)];
	wcstombs(nickName, NickName, NICKNAME_LEN * sizeof(TCHAR));

	
	int64 PlayerID = ++lastPlayerId;
	// last_player_id 테이블 업데이트
	char updateQuery[256];
	sprintf(updateQuery, "UPDATE last_player_id SET PlayerID = %lld", lastPlayerId);

	if (mysql_query(&_conn, updateQuery))
	{
		fprintf(stderr, "last_player_id query fail: %s\n", mysql_error(&_conn));
		// 실패한 경우 lastPlayerId를 다시 감소
		--lastPlayerId;

		CPacket* resPacket = CPacket::Alloc();
		uint8 Status = 0;
		MP_SC_CREATE_PLAYER(resPacket, Status, PlayerInfo{});
		SendPacket_Unicast(player->_sessionId, resPacket);
		CPacket::Free(resPacket);
		return;
	}


	// 쿼리 문자열 생성
	char query[1024];
	sprintf(query, "INSERT INTO Player(PlayerID, AccountNo, NickName, Class) VALUES(%lld, %lld, '%s', %d)", PlayerID, player->accountNo, nickName, Class);

	// 쿼리 실행
	bool createSuccess = true;
	if (mysql_query(&_conn, query))
	{
		fprintf(stderr, "쿼리 실행 실패: %s\n", mysql_error(&_conn));
		createSuccess = false;
	}

	//TODO: 플레이어 info에 넣기
	PlayerInfo playerInfo;
	playerInfo.PlayerID = PlayerID;
	wcscpy(playerInfo.NickName, NickName);
	playerInfo.Class = Class;
	playerInfo.Level = 1;
	playerInfo.Exp = 0;
	player->playerInfos.push_back(playerInfo);
	
	CPacket* resPacket = CPacket::Alloc();
	uint8 Status = createSuccess ? 1 : 0;
	MP_SC_CREATE_PLAYER(resPacket, Status, playerInfo);
	SendPacket_Unicast(player->_sessionId, resPacket);
	CPacket::Free(resPacket);
	
	printf("HandleCreatePlayer \n");
}






