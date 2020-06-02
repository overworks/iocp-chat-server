#pragma once

#include "Packet.h"
#include <string>

constexpr size_t PACKET_DATA_BUFFER_SIZE = 8096u;

class User
{
public:
	enum class DomainState
	{
		None,
		Login,
		Room
	};

	User(int index);
	User(const User&) = delete;
	~User();

	void Reset();
	void Login(const std::string& user_id);
	void EnterRoom(int room_number);
	void LeaveRoom();

	void SetPacketData(size_t data_size, const char* data);
	PacketInfoPtr GetPacket();

	int GetIndex() const { return m_index; }
	int GetRoomNumber() const { return m_room_number; }
	const std::string& GetUserID() const { return m_user_id; }

	bool IsLoggedIn() const { return !m_user_id.empty(); }
	bool IsInRoom() const { return m_room_number != -1; }

private:
	int m_index = -1;
	int m_room_number = -1;

	std::string m_user_id;
	std::string m_auth_token;

	char m_buffer[PACKET_DATA_BUFFER_SIZE];
	size_t m_buffer_write_pos; // 버퍼에 쓰기 시작할 위치
	size_t m_buffer_read_pos;  // 버퍼에서 읽어들일 위치
};
