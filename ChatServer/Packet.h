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
	Mh::u16 length;
	PacketID packet_id;
	Mh::u8 type;
	
	PacketHeader() {}
	PacketHeader(PacketID packet_id, Mh::u16 length)
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
	char user_id[MAX_USER_ID_LEN + 1];
	char password[MAX_USER_PW_LEN + 1];

	PacketLoginReq()
		: PacketHeader(PacketID::LOGIN_REQUEST, sizeof(*this))
	{}
};

struct PacketLoginRes : public PacketHeader
{
	ERROR_CODE result;

	PacketLoginRes()
		: PacketLoginRes(ERROR_CODE::NONE)
	{}

	PacketLoginRes(ERROR_CODE result)
		: PacketHeader(PacketID::LOGIN_RESPONSE, sizeof(*this)), result(result)
	{}
};

//- 룸에 들어가기 요청
//const int MAX_ROOM_TITLE_SIZE = 32;
struct PacketRoomEnterReq : public PacketHeader
{
	Mh::s16 room_number;

	PacketRoomEnterReq()
		: PacketHeader(PacketID::ROOM_ENTER_REQUEST, sizeof(*this))
	{}
};

struct PacketRoomEnterRes : public PacketHeader
{
	ERROR_CODE result;

	
	PacketRoomEnterRes()
		: PacketRoomEnterRes(ERROR_CODE::NONE)
	{}

	PacketRoomEnterRes(ERROR_CODE result = ERROR_CODE::NONE)
		: PacketHeader(PacketID::ROOM_ENTER_RESPONSE, sizeof(*this)), result(result)
	{}
};


//- 룸 나가기 요청
struct PacketRoomLeaveReq : public PacketHeader
{
	PacketRoomLeaveReq()
		: PacketHeader(PacketID::ROOM_LEAVE_REQUEST, sizeof(*this))
	{}
};

struct PacketRoomLeaveRes : public PacketHeader
{
	ERROR_CODE result;

	PacketRoomLeaveRes()
		: PacketRoomLeaveRes(ERROR_CODE::NONE)
	{}

	PacketRoomLeaveRes(ERROR_CODE result)
		: PacketHeader(PacketID::ROOM_LEAVE_RESPONSE, sizeof(*this)), result(result)
	{}
};



// 룸 채팅
constexpr int MAX_CHAT_MSG_SIZE = 256;
struct PacketRoomChatReq : public PacketHeader
{
	char message[MAX_CHAT_MSG_SIZE + 1] = { 0, };

	PacketRoomChatReq()
		: PacketHeader(PacketID::ROOM_CHAT_REQUEST, sizeof(*this))
	{}
};

struct PacketRoomChatRes : public PacketHeader
{
	ERROR_CODE result;

	PacketRoomChatRes()
		: PacketRoomChatRes(ERROR_CODE::NONE)
	{}

	PacketRoomChatRes(ERROR_CODE result)
		: PacketHeader(PacketID::ROOM_CHAT_RESPONSE, sizeof(*this)), result(result)
	{}
};

struct PacketRoomNotify : public PacketHeader
{
	char user_id[MAX_USER_ID_LEN + 1] = { 0, };
	char message[MAX_CHAT_MSG_SIZE + 1] = { 0, };

	PacketRoomNotify()
		: PacketHeader(PacketID::ROOM_CHAT_NOTIFY, sizeof(*this))
	{}

	PacketRoomNotify(const char* user_id, const char* message)
		: PacketHeader(PacketID::ROOM_CHAT_NOTIFY, sizeof(*this))
	{
		std::copy(user_id, user_id + sizeof(this->user_id), this->user_id);
		std::copy(message, message + sizeof(this->message), this->message);
	}
};

#pragma pack(pop)
