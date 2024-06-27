#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

using int64 = signed long long;
using int32 = int;
using int16 = short;
using uint32 = unsigned int;
using uint16 = unsigned short;
using uint64 = unsigned long long;
using uint8 = unsigned char;
using FVector = struct FVector
{
	double X;
	double Y;
	double Z;
};