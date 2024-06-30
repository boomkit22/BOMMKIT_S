#pragma once

#include "Type.h"



//#define PACKET_CODE 0x77
//#define PACKET_KEY 0x32


#define SESSION_KEY_LEN 64
#define TIMEOUT_DISCONNECT 40000
#define GAME_SERVER_NO 3


enum PACKET_TYPE
{
	//------------------------------------------------------
	// Login Server
	//------------------------------------------------------
	PACKET_LOGIN_SERVER = 0,

	//------------------------------------------------------------
	// 로그인 서버로 클라이언트 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		TCHAR   ID[20]
	//		TCHAR	PassWord[20]     //사용자 PassWord. null포함
	//	}
	//------------------------------------------------------------
	PACKET_CS_LOGIN_REQ_LOGIN,

	//------------------------------------------------------------
	// 로그인 서버에서 클라이언트로 로그인 응답
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		BYTE	Status				// 0 (세션오류) / 1 (성공) ...  하단 defines 사용
	//
	// 
	//		WCHAR	GameServerIP[16]	//	접속대상 게임,채팅 서버 정보
	//		USHORT	GameServerPort
	//		WCHAR	ChatServerIP[16]
	//		USHORT	ChatServerPort
	//	}
	//
	//------------------------------------------------------------
	PACKET_SC_LOGIN_RES_LOGIN,


	//------------------------------------------------------------
	//
	//	{
	//		WORD	Type
	//	}
	//------------------------------------------------------------
	PACKET_CS_LOGIN_REQ_ECHO,

	//------------------------------------------------------------
	//	{
	//		WORD	Type
	//	}
	//------------------------------------------------------------
	PACKET_SC_LOGIN_RES_ECHO,

	//------------------------------------------------------
	// Game Server
	//------------------------------------------------------
	PACKET_GAME_SERVER = 1000,

	//------------------------------------------------------------
	//	{
	//		WORD	Type
	//		INT64	AccountNo
	//	}
	//------------------------------------------------------------
	PACKET_CS_GAME_REQ_LOGIN = 1001,


	//------------------------------------------------------------
	//	{
	//		WORD	Type
	//		ResLoginInfo resLoginInfo
	// 
	//	}
	//------------------------------------------------------------
	PACKET_SC_GAME_RES_LOGIN = 1002,

	//------------------------------------------------------------
	//	{
	//		WORD	Type
	//		uint16  fieldID
	//	}
	//------------------------------------------------------------
	PACKET_CS_GAME_REQ_FIELD_MOVE = 1003,
	
	
	
	//------------------------------------------------------------
	//	{
	//		WORD	Type
	//		uint8   Status    
	//	}
	//------------------------------------------------------------
	PACKET_SC_GAME_RES_FIELD_MOVE = 1004,

	//------------------------------------------------------------
	//	{
	//		WORD	Type
	//		SpawnMyCharacterInfo spawnMyCharacteRInfo
	//	}
	//------------------------------------------------------------
	PACKET_SC_GAME_SPAWN_MY_CHARACTER = 1005,


	//------------------------------------------------------------
	//	{
	//		WORD	Type
	//		SpawnOtherCharacterInfo spawnOtherCharacterInfo
	//	}
	//------------------------------------------------------------
	PACKET_SC_GAME_SPAWN_OTHER_CHAACTER = 1006,

	//------------------------------------------------------------
	//	 이거 필요한가
	//		WORD	Type
	//------------------------------------------------------------
	PACKET_SC_GAME_DESPAWN_MY_CHARACTER = 1007,


	//------------------------------------------------------------
	//	 
	//	WORD	Type
	//  int64 CharacterNO
	//------------------------------------------------------------
	PACKET_SC_GAME_DESPAWN_OTHER_CHARACTER = 1008,


	//------------------------------------------------------------
	// {
	//		WORD	Type
	//		FVector Destination
	//	}
	//------------------------------------------------------------
	PACKET_CS_GAME_REQ_CHARACTER_MOVE = 1009,


	//------------------------------------------------------------
	// {
	//		WORD	Type
	//		int64 CharacterNO
	//		FVector Destination
	//	}
	//------------------------------------------------------------
	PACKET_SC_GAME_RES_CHARACTER_MOVE = 1010,

	//------------------------------------------------------------
	// {
	//		WORD	Type
	//		int32   AttackerType
	//		int64   AttackerID
	//		int32   TargetType
	// 		int64   TargetID
	// 		int32   Damage
	//	}
	//------------------------------------------------------------
	PACKET_CS_GAME_REQ_CHARACTER_ATTACK = 1011,


	//------------------------------------------------------------
	// {
	//		WORD	Type
	//		int32   AttackerType
	//		int64   AttackerID
	//		int32   TargetType
	// 		int64   TargetID
	// 		int32   Damage
	//	}
	//------------------------------------------------------------
	PACKET_SC_GAME_RES_DAMAGE = 1012,

	//------------------------------------------------------------
	// {
	//		WORD	Type
	//      FRotator StartRotation
	//		int32   SkillID
	//}
	//------------------------------------------------------------
	PACKET_CS_GAME_REQ_CHARACTER_SKILL = 1013,

	//------------------------------------------------------------
	// {
	//		WORD	Type
	//		int64   CharacterNO
	//		FRotator StartRotation
	//		int32   SkillID
	//
	// }
	//------------------------------------------------------------
	PACKET_SC_GAME_RES_CHARACTER_SKILL = 1014,

	//------------------------------------------------------------
	// {
	//		WORD	Type
	//		int64   MonsterNO
	//		int32   SkillID
	// }
	//------------------------------------------------------------
	PACKET_SC_GAME_RES_MONSTER_SKILL = 1015,
};

