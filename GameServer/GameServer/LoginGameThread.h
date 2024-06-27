#pragma once
#include "GameThread.h"
#include <map>
#include "Player.h"
#include "LockFreeObjectPool.h"
#include "GameServer.h"

class LoginGameThread : public GameThread
{
public:
	LoginGameThread(GameServer* gameServer, int threadId);
	
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
	void MP_SC_LOGIN(CPacket* packet, int64& accountNo, uint8& status, uint16& characterLevel);

private:
	std::unordered_map<int64, Player*> _playerMap;

	virtual void HandleRecvPacket(int64 sessionId, std::vector<CPacket*>& packets) override;
	uint16 serverPacketCode = Data::serverPacketCode;

};

