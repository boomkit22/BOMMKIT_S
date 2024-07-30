#include "FieldObject.h"
#include "Sector.h"
#include "GameServer.h"
#include "GameData.h"

using namespace std;

FieldObject::FieldObject(FieldPacketHandleThread* field, uint16 objectType)
	: _field(field), _objectType(objectType)
{
	static int64 ojbectIdGenerator = 0;
	_objectId = ojbectIdGenerator++;
}

FieldObject::~FieldObject()
{

}

void FieldObject::SendPacket_Unicast(int64 objectId, CPacket* packet)
{
	_field->SendPacket(objectId, packet);
}

void FieldObject::SendPacket_Around(CPacket* packet, bool bInclude)
{
	int aroundSectorNum = _currentSector->aroundSectorNum;
	Sector** around = _currentSector->_around;

	// 플레이어인 경우
	if (_objectType == TYPE_PLAYER)
	{
		//자신 포함하고 보내면
		if (bInclude)
		{
			for (int i = 0; i < aroundSectorNum; i++)
			{
				vector<Player*>& gcList = around[i]->playerVector;

				for (Player* p : gcList)
				{
					SendPacket_Unicast(p->_objectId, packet);
				}
			}
		}
		else {
			// 주변에 보내고
			for (int i = 0; i < aroundSectorNum - 1; i++)
			{
				vector<Player*>& gcList = around[i]->playerVector;

				for (Player* p : gcList)
				{
					SendPacket_Unicast(p->_objectId, packet);
				}
			}

			// 자신것 따로 보내고
			Sector* sector =_currentSector;
			vector<Player*>& gcList = sector->playerVector;
			for (Player* p : gcList)
			{
				if (p->_objectId == _objectId)
				{
					continue;
				}
				SendPacket_Unicast(p->_objectId, packet);
			}
		}
	} 
	else {
		// 몬스터인 경우
		for (int i = 0; i < aroundSectorNum; i++)
		{
			vector<Player*>& gcList = around[i]->playerVector;

			for (Player* p : gcList)
			{
				SendPacket_Unicast(p->_objectId, packet);
			}
		}
	}
}

FieldObject* FieldObject::FindFieldObject(int64 objectId)
{
	return _field->FindFieldObject(objectId);
}
