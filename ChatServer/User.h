#pragma once

#include <string>
#include "Types.h"

class User
{
public:
	User(int index);

	int GetIndex() const { return m_index; }
	void EnterRoom(int room_index) { m_room_index = room_index; }
	int GetRoomIndex() const { return m_room_index; }

private:
	int m_index = -1;
	int m_room_index = -1;
	std::string m_user_id;
};
