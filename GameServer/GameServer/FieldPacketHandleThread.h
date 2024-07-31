#pragma once
#include "BasePacketHandleThread.h"
#include "Type.h"

class CPacket;
class Player;
class GameServer;
class Monster;
class Sector;
class FieldObject;

//같은거
//  ReqFieldMvoe
//  ReqCharacterMove
//  ReqCharacterSkill
//  ReqCharacterStop

class FieldPacketHandleThread : public BasePacketHandleThread
{
public:
	FieldPacketHandleThread(GameServer* gameServer, int threadId, int msPerFrame, 
		uint16 sectorYLen, uint16 sectorXLen, uint16 sectorYSize, uint16 sectorXSize);

protected:
	void HandleFieldMove(Player* player, CPacket* packet);
	void HandleChracterMove(Player* player, CPacket* packet);
	void HandleCharacterSkill(Player* player, CPacket* packet);
	void HandleCharacterStop(Player* player, CPacket* packet);
	void HnadleCharacterAttack(Player* player, CPacket* packet);

private:
	virtual void GameRun(float deltaTime) override;
	virtual void FrameUpdate(float deltaTime) = 0;
	virtual void OnEnterThread(int64 sessionId, void* ptr) override;
	virtual void OnLeaveThread(int64 sessionId, bool disconnect) override;

public:
	FieldObject* FindFieldObject(int64 objectId);
	void ReturnFieldObject(int64 objectId);
	Monster* AllocMonster(uint16 monsterType);

protected:
	std::unordered_map<int64, Monster*> _monsterMap;
	std::unordered_map<int64, FieldObject*> _fieldObjectMap;

private:
	CObjectPool<Monster, true> _monsterPool;

protected:
	//섹터
	uint16 _sectorYLen;
	uint16 _sectorXLen;
	uint16 _sectorYSize;
	uint16 _sectorXSize;
	uint32 _sectorYSizeTotal;
	uint32 _sectorXSizeTotal;

	Sector** _sector;
	void InitializeSector();

public:
	Sector* GetSector(uint16 newSectorY, uint16 newSectorX);
};

