#include "PacketHeader.h"
#include "Packet.h"
#include "SerializeBuffer.h"
#include "Type.h"
#include "EchoGameThread.h"



void EchoGameThread::MP_SC_LOGIN(CPacket* packet, uint8& status, int64& accountNo)
{
	//NetHeader header;
	//header._code = serverPacketCode;
	//header._randKey = rand();
	//packet->PutData((char*)&header, sizeof(NetHeader));

	//uint16 type = PACKET_CS_GAME_RES_LOGIN;
	//*packet << type;
	//*packet << status;
	//*packet << accountNo;

	//uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	//memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}



void  EchoGameThread::MP_SC_ECHO(CPacket* packet, CPacket* echoPacket)
{
	//uint16 packetType;
	//int64 recvAccountNo;
	//int64 sendTick;
	//*echoPacket >> packetType >> recvAccountNo >> sendTick;

	//NetHeader header;
	//header._code = serverPacketCode;
	//header._randKey = rand();
	//packet->PutData((char*)&header, sizeof(NetHeader));

	//uint16 type = PACKET_CS_GAME_RES_ECHO;
	//*packet << type << recvAccountNo << sendTick;
	////*packet << recvAccountNo;
	////*packet << sendTick;

	//uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	//memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}

void EchoGameThread::MP_SC_FIELD_MOVE(CPacket* packet, uint8& status)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_RES_FIELD_MOVE;
	*packet << type << status;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}


void EchoGameThread::MP_SC_SPAWN_MY_CHARACTER(CPacket* packet, FVector& SpawnLocation)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_SPAWN_MY_CHARACTER;
	*packet << type << SpawnLocation;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}
