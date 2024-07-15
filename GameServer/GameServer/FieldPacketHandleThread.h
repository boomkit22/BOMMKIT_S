#pragma once
#include "BasePacketHandleThread.h"
#include "Type.h"
class CPacket;
class Player;
class GameServer;

//°°Àº°Å
//  ReqFieldMvoe
//  ReqCharacterMove
//  ReqCharacterSkill
//  ReqCharacterStop

class FieldPacketHandleThread : public BasePacketHandleThread
{
public:
	FieldPacketHandleThread(GameServer* gameServer, int threadId);

protected:
	void HandleFieldMove(Player* player, CPacket* packet);
	void HandleChracterMove(Player* player, CPacket* packet);
	void HandleCharacterSkill(Player* player, CPacket* packet);
	void HandleCharacterStop(Player* player, CPacket* packet);


};

