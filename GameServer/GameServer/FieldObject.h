#pragma once
#include "Type.h"

class FieldPacketHandleThread;
class Sector;

class FieldObject
{
public:
	FieldObject(FieldPacketHandleThread* field, uint16 objectType);
	virtual ~FieldObject();
	void SendPacket_Unicast(int64 objectId, CPacket* packet);
	void SendPacket_Around(CPacket* packet, bool bInclude = true);

	FieldObject* FindFieldObject(int64 objectId);
	void SetField(FieldPacketHandleThread* field) { _field = field; };

	virtual void Update(float deltaTime) = 0;
	uint16 GetObjectType() { return _objectType; };

protected:
	int64 _objectId;

private:
	uint16 _objectType;
	Sector* _currentSector;
	FieldPacketHandleThread* _field;
};

