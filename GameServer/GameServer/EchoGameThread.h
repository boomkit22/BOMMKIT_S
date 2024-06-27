#pragma once
#include "GameThread.h"
#include <map>
#include "Player.h"
#include "GameServer.h"

class EchoGameThread : public GameThread
{
public:
	EchoGameThread(GameServer* gameServer, int threadId);

public:
	int64 GetPlayerSize() override
	{
		return _playerMap.size();
	}

private:
	GameServer* _gameServer;

public:
	// GameThread을(를) 통해 상속됨
	void HandleRecvPacket(int64 sessionId, CPacket* packet) override;
	void HandleRecvPacket(int64 sessionId, std::vector<CPacket*>& packets) override;
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;

private:
	std::unordered_map<int64, Player*> _playerMap;

private:
	uint16 serverPacketCode = Data::serverPacketCode;
	void MP_SC_LOGIN(CPacket* packet, uint8& status, int64& accountNo);
	void MP_SC_ECHO(CPacket* packet, CPacket* echoPacket);
};

