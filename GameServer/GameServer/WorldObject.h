#pragma once
#include "Type.h"

class GameServer;
class Sector;

class WorldObject
{
public:
	WorldObject(GameServer* gameServer);
	virtual ~WorldObject();
	void SendPacket_Unicast(int64 objectId, CPacket* packet);
	void SendPacket_Around(CPacket* packet, bool bInclude);

private:
	Sector* _sector;
	int64 _objectId;
	GameServer* _gameServer;
};

