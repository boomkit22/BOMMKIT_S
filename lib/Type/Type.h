#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#define ID_LEN 20
#define NICKNAME_LEN 20

class CPacket;
using int64 = signed long long;
using int32 = int;
using int16 = short;
using uint32 = unsigned int;
using uint16 = unsigned short;
using uint64 = unsigned long long;
using uint8 = unsigned char;

using TCHAR = WCHAR;

struct FVector
{
	double X;
	double Y;
	double Z;
};

CPacket& operator<<(CPacket& packet, FVector& vec);
CPacket& operator>>(CPacket& packet, FVector& vec);


struct ResGameLoginInfo
{
	int64 AccountNo;
	uint8 Status;
	uint16 Level;
	TCHAR NickName[ID_LEN];
};

CPacket& operator<<(CPacket& packet, ResGameLoginInfo& resLoginInfo);
CPacket& operator>>(CPacket& packet, ResGameLoginInfo& resLoginInfo);


struct SpawnMyCharacterInfo
{
	FVector SpawnLocation;
	int64 PlayerID;
	uint16 Level;
	TCHAR NickName[ID_LEN];
};

CPacket& operator<<(CPacket& packet, SpawnMyCharacterInfo& spawnMyCharacterInfo);
CPacket& operator>>(CPacket& packet, SpawnMyCharacterInfo& spawnMyCharacterInfo);


struct SpawnOtherCharacterInfo
{
	FVector SpawnLocation;
	int64 PlayerID;
	uint16 Level;
	TCHAR NickName[ID_LEN];
};

CPacket& operator<<(CPacket& packet, SpawnOtherCharacterInfo& spawnOtherCharacterInfo);
CPacket& operator>>(CPacket& packet, SpawnOtherCharacterInfo& spawnOtherCharacterInfo);






