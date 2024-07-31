#pragma once
#include <map>
#include <vector>
#include "ObjectPool.h"
#include "FieldPacketHandleThread.h"

class Player;
class CPacket;
class GameServer;

class LobbyFieldThread : public FieldPacketHandleThread
{
public:
	LobbyFieldThread(GameServer* gameServer, int threadId, int msPerFrame,
		uint16 sectorYLen, uint16 sectorXLen, uint16 sectorYSize, uint16 sectorXSize);

private:
	virtual void GameRun(float deltaTime) override;
	// GameThread��(��) ���� ��ӵ�
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;

private:
	void HandleCharacterAttack(Player* p, CPacket* packet);
	void UpdatePlayers(float deltaTime);


public:
	// FieldPacketHandleThread��(��) ���� ��ӵ�
	void FrameUpdate(float deltaTime) override;
};

