#include "WorldObject.h"

WorldObject::WorldObject(GameServer* gameServer)
{
}

WorldObject::~WorldObject()
{
}

void WorldObject::SendPacket_Unicast(int64 objectId, CPacket* packet)
{
}

void WorldObject::SendPacket_Around(CPacket* packet, bool bInclude)
{
}
