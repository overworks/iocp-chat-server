#include "User.h"
#include "Room.h"
#include "RoomManager.h"

RoomManager::~RoomManager()
{
	Release();
}

void
RoomManager::Init(size_t max_user_count, size_t max_room_count, int begin_room_number)
{
	m_max_user = max_user_count;
	m_begin_room_number = begin_room_number;

	m_room_list.reserve(max_room_count);
	for (int i = 0, imax = static_cast<int>(max_room_count); i < imax; ++i)
	{
		int room_number = begin_room_number + i;
		m_room_list.push_back(new Room(room_number));
	}
}

void
RoomManager::Release()
{
	for (auto room : m_room_list)
	{
		delete room;
	}
	m_room_list.clear();
}

Room*
RoomManager::GetRoom(int room_number)
{
	if (room_number < m_begin_room_number)
	{
		return nullptr;
	}

	size_t index = static_cast<size_t>(room_number - m_begin_room_number);
	if (index >= m_room_list.size())
	{
		return nullptr;
	}

	return m_room_list[index];
}

ERROR_CODE
RoomManager::EnterUser(int room_number, User* user)
{
	Room* room = GetRoom(room_number);
	if (!room)
	{
		return ERROR_CODE::ROOM_INVALID_INDEX;
	}

	if (room->GetCurrentUserCount() >= m_max_user)
	{
		return ERROR_CODE::ENTER_ROOM_FULL_USER;
	}

	return room->EnterUser(user);
}

ERROR_CODE
RoomManager::LeaveUser(User* user)
{
	int room_number = user->GetRoomNumber();
	Room* room = GetRoom(room_number);
	if (!room)
	{
		return ERROR_CODE::ROOM_INVALID_INDEX;
	}

	room->LeaveUser(user);
	return ERROR_CODE();
}
