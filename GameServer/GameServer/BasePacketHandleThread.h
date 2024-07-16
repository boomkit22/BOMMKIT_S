#pragma once
#include "Type.h"
#include "GameThread.h"
#include <map>
#include <vector>
#include "ObjectPool.h"
#include "Monster.h"
#include <functional>
class CPacket;
class Player;
class GameServer;

class BasePacketHandleThread : public GameThread
{
public:
	BasePacketHandleThread(GameServer* gameServer, int threadId);

public:
	int64 GetPlayerSize() override;
	void SendPacket_Unicast(int64 sessionId, CPacket* packet);
	void SendPacket_BroadCast(CPacket* packet, Player* p = nullptr);

protected:	
	// GameThread을(를) 통해 상속됨
	void HandleRecvPacket(int64 sessionId, CPacket* packet) override;
	Player* AllocPlayer(int64 sessionId);
	void FreePlayer(int64 sessionId);


protected: // PacketHandler 
	using PacketHandler = std::function<void(Player*, CPacket*)>;
	std::map<uint16, PacketHandler> _packetHandlerMap;
	void RegisterPacketHandler(uint16 packetCode, PacketHandler handler);

	std::unordered_map<int64, Player*> _playerMap;


protected:
	GameServer* _gameServer;
};

