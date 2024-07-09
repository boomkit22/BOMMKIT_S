#include "PacketHeader.h"
#include "Packet.h"
#include "SerializeBuffer.h"
#include "Type.h"
#include "GameGameThread.h"

void LoginGameThread::MP_SC_LOGIN(CPacket* packet, uint8 Status, int64 PlayerId, uint16 CharacterLevel, TCHAR* NickName)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_RES_LOGIN;
	*packet << type << Status << PlayerId << CharacterLevel << NickName;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}

void LoginGameThread::MP_SC_FIELD_MOVE(CPacket* packet, uint8& status)
{
}
