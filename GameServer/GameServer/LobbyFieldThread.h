#pragma once
#include "GameThread.h"
#include <map>
#include "Player.h"
#include "GameServer.h"
#include <vector>
#include "ObjectPool.h"
#include "Monster.h"

class LobbyFieldThread : public GameThread
{
public:
	LobbyFieldThread(GameServer* gameServer, int threadId);

public:
	int64 GetPlayerSize() override
	{
		return _playerMap.size();
	}

private:
	GameServer* _gameServer;
	std::unordered_map<int64, Player*> _playerMap;

private:
	virtual void GameRun(float deltaTime) override;



public:
	// GameThread을(를) 통해 상속됨
	void HandleRecvPacket(int64 sessionId, CPacket* packet) override;
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;

	void SendPacket(int64 sessionId, CPacket* packet);


private:
	void HandleCharacterMove(Player* p, CPacket* packet);
	void HandleCharacterAttack(Player* p, CPacket* packet);
	void HandleCharacterSkill(Player* p, CPacket* packet);
	void HandleCharacterStop(Player* p, CPacket* packet);
	void HandleFieldMove(Player* p, CPacket* packet);

	void UpdatePlayers(float deltaTime);
private:
	uint16 serverPacketCode = Data::serverPacketCode;



	// GameThread을(를) 통해 상속됨
	virtual void SendPacket_BroadCast(CPacket* packet) override;

};

