#pragma once

#include <vector>
#include "ErrorCode.h"

class User;
class Room;

class RoomManager
{
public:
	RoomManager() = default;
	RoomManager(const RoomManager&) = delete;
	~RoomManager();

	void Init(size_t max_user_count, size_t max_room_count, int begin_room_number);
	void Release();

	Room* GetRoom(int room_number);

	ERROR_CODE EnterUser(int room_number, User* user);
	ERROR_CODE LeaveUser(User* user);

private:
	size_t m_max_user;
	int m_begin_room_number;
	std::vector<Room*> m_room_list;
};