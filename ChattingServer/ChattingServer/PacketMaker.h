#pragma once
class CPacket;

void MP_SC_CHAT_MESSAGE(CPacket* packet, int64& accountNo, WCHAR* id, WCHAR* nickName, uint16& messageLen, CPacket* message);
void MP_SC_LOGIN(CPacket* packet, uint8& status, int64& accountNo);
void MP_SC_SECTOR_MOVE(CPacket* packet, int64& accountNo, uint16& sectorX, uint16& sectorY);

void MP_SC_MONITOR_TOOL_DATA_UPDATE(CPacket* packet, 
	uint8& dataType, int& dataValue, int& timeStamp);

void MP_SS_MONITOR_LOGIN(CPacket* packet, int& serverNo);