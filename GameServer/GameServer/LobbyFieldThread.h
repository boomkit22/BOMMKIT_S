#pragma once
#include <map>
#include <vector>
#include "ObjectPool.h"
#include "BasePacketHandleThread.h"

class Player;
class CPacket;
class GameServer;

class LobbyFieldThread : public BasePacketHandleThread
{
public:
	LobbyFieldThread(GameServer* gameServer, int threadId);

private:
	virtual void GameRun(float deltaTime) override;

public:
	// GameThread을(를) 통해 상속됨
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;

private:
	void HandleCharacterAttack(Player* p, CPacket* packet);
	void UpdatePlayers(float deltaTime);
};

