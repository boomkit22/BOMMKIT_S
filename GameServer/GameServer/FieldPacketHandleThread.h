#pragma once
#include "BasePacketHandleThread.h"
#include "Type.h"
#include "JumpPointSearch.h"
#include <thread>
#include <condition_variable>
#include "mysql.h"
#include "Data.h"
#include <queue>


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
		uint16 sectorYLen, uint16 sectorXLen, uint16 sectorYSize, uint16 sectorXSize, uint8** map);

	~FieldPacketHandleThread();

protected:
	void HandleFieldMove(Player* player, CPacket* packet);
	//void HandleChracterMove(Player* player, CPacket* packet);
	void HandleCharacterSkill(Player* player, CPacket* packet);
	void HandleCharacterStop(Player* player, CPacket* packet);
	void HnadleCharacterAttack(Player* player, CPacket* packet);
	//void HandleCharacterNextPath(Player* player, CPacket* packet);
	void HandleFindPath(Player* player, CPacket* packet);


protected:
	void HandleAsyncFindPath(Player* player);

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

	Sector** _sector;
	void InitializeSector();

public:
	Sector* GetSector(uint16 newSectorY, uint16 newSectorX);
	uint32 GetMapXSize() { return _mapSizeX; };
	uint32 GetMapYSize() { return _mapSizeY; };
	bool CheckValidPos(Pos pos);
private:
	//HandleCharacterMove 요청왔을때 길찾기하는 쓰레드 만들기
	uint8** _map;
	void InitializeMap();
	uint32 _mapSizeX;
	uint32 _mapSizeY;
	JumpPointSearch* _playerJps;
	JumpPointSearch* _monsterJps;



private:
	// BasePacketHandleThread을(를) 통해 상속됨
	//virtual void HandleAsyncJobFinish(int64 sessionId) override;

public:
	//void RequestFindPath(int64 objectId, Pos start, Pos dest);
	void RequestMonsterPath(Monster* monster, Pos start, Pos dest);

	// BasePacketHandleThread을(를) 통해 상속됨
	void HandleAsyncJobFinish(void* ptr, uint16 jobType) override;

	//DB
public:
	void AddDBJob(std::function<void()> job);
	MYSQL* GetDBConnection() { return &_conn; };
private:
	bool bThreadRun = true;
	std::thread _dbThread;
	std::condition_variable _dbCV;
	std::mutex _dbMutex;
	void DBThreadFunc();
	std::queue<std::function<void()>> _dbJobQueue;
	//mysql
	//TODO: 비밀번호 ip config로 뺴기
	MYSQL _conn;
	MYSQL* _connection;
	const char* host = "127.0.0.1";
	const char* user = "root";
	const char* password = "boomkit22";
	const char* database = "mmo";
	int port = Data::DBPort;
};

