#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#define ID_LEN 20
#define NICKNAME_LEN 20
#define PASS_LEN 20

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

struct FRotator
{
	double Pitch;
	double Yaw;
	double Roll;
};

CPacket& operator<<(CPacket& packet, FRotator& rot);
CPacket& operator>>(CPacket& packet, FRotator& rot);

struct PlayerInfo
{
	int64 PlayerID;
	TCHAR NickName[NICKNAME_LEN];
	uint16 Class;
	uint16 Level;
	uint32 Exp;
	int32 Hp;
};

CPacket& operator<<(CPacket& packet, PlayerInfo& info);
CPacket& operator>>(CPacket& packet, PlayerInfo& info);

struct MonsterInfo
{
	int64 MonsterID;
	uint16 Type;
	int32 Hp;
};

CPacket& operator<<(CPacket& packet, MonsterInfo& info);
CPacket& operator>>(CPacket& packet, MonsterInfo& info);