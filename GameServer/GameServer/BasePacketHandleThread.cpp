#include "BasePacketHandleThread.h"
#include "SerializeBuffer.h"
#include "Packet.h"
#include "GameServer.h"

BasePacketHandleThread::BasePacketHandleThread(GameServer* gameServer, int threadId) : GameThread(threadId, 100)
{
	_gameServer = gameServer;
	SetGameServer((CNetServer*)gameServer);
}

void BasePacketHandleThread::RegisterPacketHandler(uint16 packetCode, PacketHandler handler)
{
	_packetHandlerMap[packetCode] = handler;
}



int64 BasePacketHandleThread::GetPlayerSize()
{
	return int64();
}

void BasePacketHandleThread::HandleRecvPacket(int64 sessionId, CPacket* packet)
{
	Player* player = nullptr;
	auto it = _playerMap.find(sessionId);
	if (it == _playerMap.end())
	{
		LOG(L"GuardianFieldThread", LogLevel::Error, L"Cannot find sessionId : %lld, HandleRecvPacket", sessionId);
		return;
	}
	player = it->second;

	uint16 packetType;
	*packet >> packetType;

	auto handlerIt = _packetHandlerMap.find(packetType);
	if (handlerIt != _packetHandlerMap.end())
	{
		handlerIt->second(player, packet);
	}
	else
	{
		//TODO: 플레이어 끊어버리기
		__debugbreak();
	}


	CPacket::Free(packet);
}

void BasePacketHandleThread::SendPacket_Unicast(int64 sessionId, CPacket* packet)
{
	_gameServer->SendPacket(sessionId, packet);
}

void BasePacketHandleThread::SendPacket_BroadCast(CPacket* packet, Player* p)
{
	for (auto it = _playerMap.begin(); it != _playerMap.end(); ++it)
	{
		if (it->second == p)
		{
			continue;
		}
		_gameServer->SendPacket(it->first, packet);
	}
}

Player* BasePacketHandleThread::AllocPlayer(int64 sessionId)
{
	return _gameServer->AllocPlayer(sessionId);

}

void BasePacketHandleThread::FreePlayer(int64 sessionId)
{
	_gameServer->FreePlayer(sessionId);
}

