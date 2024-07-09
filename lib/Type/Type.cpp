#include "Type.h"
#include "SerializeBuffer.h"

CPacket& operator<<(CPacket& packet, FRotator& rot)
{
	packet << rot.Pitch << rot.Yaw << rot.Roll;
	return packet;
}

CPacket& operator>>(CPacket& packet, FRotator& rot)
{
	packet >> rot.Pitch >> rot.Yaw >> rot.Roll;
	return packet;
}

CPacket& operator<<(CPacket& packet, FVector& vec)
{
	packet << vec.X << vec.Y << vec.Z;
	return packet;
}

CPacket& operator>>(CPacket& packet, FVector& vec)
{
	packet >> vec.X >> vec.Y >> vec.Z;
	return packet;
}
