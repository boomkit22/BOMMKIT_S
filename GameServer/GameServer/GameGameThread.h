#pragma once
#include "GameThread.h"
#include <map>
#include "Player.h"
#include "GameServer.h"

class GameGameThread : public GameThread
{
public:
	GameGameThread(GameServer* gameServer, int threadId);

public:
	int64 GetPlayerSize() override
	{
		return _playerMap.size();
	}

private:
	GameServer* _gameServer;

public:
	// GameThread을(를) 통해 상속됨
	void HandleRecvPacket(int64 sessionId, CPacket* packet) override;
	void OnLeaveThread(int64 sessionId, bool disconnect) override;
	void OnEnterThread(int64 sessionId, void* ptr) override;

private:
	std::unordered_map<int64, Player*> _playerMap;

private:
	void HandleCharacterMove(Player* p, CPacket* packet);
	void HandleCharacterAttack(Player* p, CPacket* packet);
	void HandleCharacterSkill(Player* p, CPacket* packet);

private:
	uint16 serverPacketCode = Data::serverPacketCode;
	void MP_SC_FIELD_MOVE(CPacket* packet, uint8& status);
	void MP_SC_SPAWN_MY_CHARACTER(CPacket* packet, int64 PlayerID, FVector SpawnLocation, uint16 Level, TCHAR* NickName);
	void MP_SC_SPAWN_OTHER_CHARACTER(CPacket* packet, int64 PlayerID, FVector SpawnLocation, uint16 Level, TCHAR* NickName);
	void MP_SC_GAME_RES_CHARACTER_MOVE(CPacket* packet, int64& charaterNo, FVector& Destination);

	
	void MP_SC_GAME_RES_DAMAGE(CPacket* packet, int32& AttackerType, int64& AttackerID, int32& targetType, 
		int64& TargetID, int32& Damage);

	void MP_SC_GAME_RES_CHARACTER_SKILL(CPacket* packet, int64& CharacterID,
		FRotator& StartRotation, int32& SkillID);

	void MP_SC_GAME_RES_MONSTER_SKILL(CPacket* packet, int64& MonsterNO, int32& SkillID);

};

