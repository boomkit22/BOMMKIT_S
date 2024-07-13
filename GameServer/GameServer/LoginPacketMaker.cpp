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

void LoginGameThread::MP_SC_PLAYER_LIST(CPacket* packet, std::vector<PlayerInfo>& playerInfos)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_RES_PLAYER_LIST;
	*packet << type << (uint8)playerInfos.size();

	for (auto& playerInfo : playerInfos)
	{
		*packet << playerInfo;
	}

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}

void LoginGameThread::MP_SC_SELECT_PLAYER(CPacket* packet, uint8& Status)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_RES_SELECT_PLAYER;
	*packet << type << Status;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));
}

void LoginGameThread::MP_SC_CREATE_PLAYER(CPacket* packet, uint8& Status, PlayerInfo playerInfo)
{
	NetHeader header;
	header._code = serverPacketCode;
	header._randKey = rand();
	packet->PutData((char*)&header, sizeof(NetHeader));

	uint16 type = PACKET_SC_GAME_RES_CREATE_PLAYER;
	*packet << type << Status << playerInfo;

	uint16 len = (uint16)(packet->GetDataSize() - sizeof(NetHeader));
	memcpy(packet->GetBufferPtr() + NET_HEADER_SIZE_INDEX, (void*)&len, sizeof(uint16));

}

void LoginGameThread::GameRun(float deltaTime)
{
}


