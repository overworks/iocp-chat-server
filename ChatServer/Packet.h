#pragma once

#include <memory>
#include "Types.h"
#include "ErrorCode.h"

enum class PacketID : Mh::u16
{
	NONE,

	//SYSTEM
	SYS_USER_CONNECT = 11,
	SYS_USER_DISCONNECT = 12,
	SYS_END = 30,

	//DB
	DB_END = 199,

	//Client
	LOGIN_REQUEST = 201,
	LOGIN_RESPONSE = 202,

	ROOM_ENTER_REQUEST = 206,
	ROOM_ENTER_RESPONSE = 207,

	ROOM_LEAVE_REQUEST = 215,
	ROOM_LEAVE_RESPONSE = 216,

	ROOM_CHAT_REQUEST = 221,
	ROOM_CHAT_RESPONSE = 222,
	ROOM_CHAT_NOTIFY = 223,
};

#pragma pack(push, 1)
struct PacketInfo
{
	Mh::u32 session_index;
	PacketID packet_id;
	Mh::u16 length;
	char* data = nullptr;
};

using PacketInfoPtr = std::shared_ptr<PacketInfo>;

struct PacketHeader
{
	PacketID packet_id;
	Mh::u16 length;

	PacketHeader() {}
	PacketHeader(PacketID packet_Id, Mh::u16 length)
		: packet_id(packet_id)
		, length(length)
	{}
};

constexpr size_t PACKET_HEADER_LENGTH = sizeof(PacketHeader);

//- 로그인 요청
constexpr int MAX_USER_ID_LEN = 32;
constexpr int MAX_USER_PW_LEN = 32;

struct PacketLoginReq : public PacketHeader
{
	char user_id[MAX_USER_ID_LEN];
	char password[MAX_USER_PW_LEN];

	PacketLoginReq()
		: PacketHeader(PacketID::LOGIN_REQUEST, sizeof(decltype(*this)))
	{}
};

struct PacketLoginRes : public PacketHeader
{
	ERROR_CODE result;

	PacketLoginRes()
		: PacketHeader(PacketID::LOGIN_RESPONSE, sizeof(decltype(*this)))
	{}
};

//- 룸에 들어가기 요청
//const int MAX_ROOM_TITLE_SIZE = 32;
struct PacketRoomEnterReq : public PacketHeader
{
	Mh::s16 room_number;

	PacketRoomEnterReq()
		: PacketHeader(PacketID::ROOM_ENTER_REQUEST, sizeof(decltype(*this)))
	{}
};

struct PacketRoomEnterRes : public PacketHeader
{
	ERROR_CODE result;

	PacketRoomEnterRes()
		: PacketHeader(PacketID::ROOM_ENTER_RESPONSE, sizeof(decltype(*this)))
	{}
};


//- 룸 나가기 요청
struct PacketRoomLeaveReq : public PacketHeader
{
	PacketRoomLeaveReq()
		: PacketHeader(PacketID::ROOM_LEAVE_REQUEST, sizeof(decltype(*this)))
	{}
};

struct PacketRoomLeaveRes : public PacketHeader
{
	ERROR_CODE result;

	PacketRoomLeaveRes()
		: PacketHeader(PacketID::ROOM_LEAVE_RESPONSE, sizeof(decltype(*this)))
	{}
};



// 룸 채팅
constexpr int MAX_CHAT_MSG_SIZE = 256;
struct PacketRoomChatReq : public PacketHeader
{
	char message[MAX_CHAT_MSG_SIZE + 1] = { 0, };

	PacketRoomChatReq()
		: PacketHeader(PacketID::ROOM_CHAT_REQUEST, sizeof(decltype(*this)))
	{}
};

struct PacketRoomChatRes : public PacketHeader
{
	ERROR_CODE result;

	PacketRoomChatRes()
		: PacketHeader(PacketID::ROOM_CHAT_RESPONSE, sizeof(decltype(*this)))
	{}
};

struct PacketRoomNotify : public PacketHeader
{
	char user_id[MAX_USER_ID_LEN + 1] = { 0, };
	char message[MAX_CHAT_MSG_SIZE + 1] = { 0, };

	PacketRoomNotify()
		: PacketHeader(PacketID::ROOM_CHAT_NOTIFY, sizeof(decltype(*this)))
	{}
};

#pragma pack(pop)
