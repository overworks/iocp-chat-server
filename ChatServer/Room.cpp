#include "User.h"
#include "Room.h"
#include "ErrorCode.h"

Room::Room(int number)
	: m_number(number)
{

}

ERROR_CODE
Room::EnterUser(User* user)
{
	auto& user_id = user->GetUserID();
	if (m_user_table.find(user_id) != m_user_table.end())
	{
		return ERROR_CODE::ENTER_ROOM_ALREADY;
	}

	m_user_table[user_id] = user;
	user->EnterRoom(GetNumber());

	return ERROR_CODE::NONE;
}

void
Room::LeaveUser(User* user)
{
	auto iter = m_user_table.find(user->GetUserID());
	if (iter != m_user_table.end())
	{
		iter->second->LeaveRoom();
		m_user_table.erase(iter);
	}
}
