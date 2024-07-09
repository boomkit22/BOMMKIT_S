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

struct FRotator
{
	float Pitch;
	float Yaw;
	float Roll;
};

CPacket& operator<<(CPacket& packet, FRotator& rot);
CPacket& operator>>(CPacket& packet, FRotator& rot);








