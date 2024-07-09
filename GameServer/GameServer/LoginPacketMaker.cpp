#include "PacketHeader.h"
#include "Packet.h"
#include "SerializeBuffer.h"
#include "Type.h"
#include "GameGameThread.h"
using namespace std;

void LoginGameThread::MP_SC_LOGIN(CPacket* packet, int64 AccountNo, uint8 Status)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_RES_LOGIN;
	*packet << type << AccountNo << Status;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}



void LoginGameThread::MP_SC_FIELD_MOVE(CPacket* packet, uint8& status)
{
}



void LoginGameThread::MP_SC_GAME_RES_SIGN_UP(CPacket* packet, uint8& Status)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_RES_SIGN_UP;
	*packet << type << Status;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}