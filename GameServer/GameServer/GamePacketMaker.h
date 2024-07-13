#pragma once
#include "Type.h"

void MP_SC_FIELD_MOVE(CPacket* packet, uint8& status);
void MP_SC_SPAWN_MY_CHARACTER(CPacket* packet, PlayerInfo playerInfo, FVector spawnLocation);
void MP_SC_SPAWN_OTHER_CHARACTER(CPacket* packet, PlayerInfo playerInfo, FVector spawnLocation);
void MP_SC_GAME_RES_CHARACTER_MOVE(CPacket* packet, int64& charaterNo, FVector& Destination, FRotator& StartRotation);

void MP_SC_GAME_RES_DAMAGE(CPacket* packet, int32& AttackerType, int64& AttackerID, int32& targetType,
	int64& TargetID, int32& Damage);

void MP_SC_GAME_RES_CHARACTER_SKILL(CPacket* packet, int64& CharacterID,
	FVector& StartLocation, FRotator& StartRotation, int32& SkillID);

void MP_SC_GAME_RES_MONSTER_SKILL(CPacket* packet, int64& MonsterNO, int32& SkillID);
void MP_SC_SPAWN_MONSTER(CPacket* packet, MonsterInfo monsterInfo, FVector spawnLocation);
void MP_SC_MONSTER_MOVE(CPacket* packet, int64& monsterId, FVector& Destination, FRotator& StartRotation);

void MP_SC_GAME_RSE_CHARACTER_STOP(CPacket* packet, int64& characterID, FVector& position, FRotator& rotation);
void MP_SC_GAME_RES_MONSTER_STOP(CPacket* packet, int64& monsterID, FVector& position, FRotator& rotation);

void MP_SC_GAME_RES_CHARACTER_DEATH(CPacket* packet, int64& characterID, FVector DeathLocation, FRotator DeathRotation);
void MP_SC_GAME_RES_MONSTER_DEATH(CPacket* packet, int64& monsterID, FVector DeathLocation, FRotator DeathRotation);