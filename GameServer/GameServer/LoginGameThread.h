#pragma once
#include "GameThread.h"
#include <map>
#include "Player.h"
#include "LockFreeObjectPool.h"
#include "GameServer.h"
#include "mysql.h"

class LoginGameThread : public GameThread
{
public:
	LoginGameThread(GameServer* gameServer, int threadId);
	~LoginGameThread();
public:
	int64 GetPlayerSize() override
	{
		return _playerMap.size();
	}

private:
	GameServer* _gameServer;

private:
	// GameThread을(를) 통해 상속됨
	void HandleRecvPacket(int64 sessionId, CPacket* packet) override;
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;

private:
	void HandleLogin(Player* player, CPacket* packet);
	void HandleFieldMove(Player* player, CPacket* packet);
	void HandleSignUp(Player* player, CPacket* packet);
	void MP_SC_LOGIN(CPacket* packet, int64 AccountNo, uint8 Status, uint16 CharacterLevel, TCHAR* NickName, uint32 Exp);
	void MP_SC_FIELD_MOVE(CPacket* packet, uint8& status);
	void MP_SC_GAME_RES_SIGN_UP(CPacket* packet, uint8& Status);


private:
	std::unordered_map<int64, Player*> _playerMap;
	uint16 serverPacketCode = Data::serverPacketCode;


private:
	//mysql
	MYSQL _conn;
	MYSQL* _connection;
	const char* host = "127.0.0.1";
	const char* user = "root";
	const char* password = "boomkit22";
	const char* database = "mmo";
	int port = Data::DBPort;
};

