#include "ChattingServer.h"
#include "PacketHeader.h"
#include "SerializeBuffer.h"
#include "Packet.h"

void ChattingServer::MP_SC_CHAT_MESSAGE(CPacket* packet, int64& accountNo, WCHAR* id, WCHAR* nickName, uint16& messageLen, CPacket* message)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));
	uint16 type = PACKET_CS_CHAT_RES_MESSAGE;
	*packet << type << accountNo;
	packet->PutData((char*)id, ID_LEN * sizeof(WCHAR));
	packet->PutData((char*)nickName, NICKNAME_LEN * sizeof(WCHAR));
	*packet << messageLen;
	//여기까지 97byte
	packet->PutData(message->GetReadPtr(), messageLen);
	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}

void ChattingServer::MP_SC_LOGIN(CPacket* packet, uint8& status, int64& accountNo)
{


	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_CS_CHAT_RES_LOGIN;
	*packet << type << status << accountNo;
	//*packet << status;
	//*packet << accountNo;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}


void ChattingServer::MP_SC_SECTOR_MOVE(CPacket* packet, int64& accountNo, uint16& sectorX, uint16& sectorY)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_CS_CHAT_RES_SECTOR_MOVE;
	*packet << type << accountNo << sectorX << sectorY;
	//*packet << accountNo;
	//*packet << sectorX;
	//*packet << sectorY;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}

