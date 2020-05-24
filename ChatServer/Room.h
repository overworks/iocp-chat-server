#pragma once

#include "Packet.h"
#include <unordered_map>

class User;
enum class ERROR_CODE;

class Room
{
public:
	Room(int number);
	Room(const Room&) = delete;
	~Room() = default;

	ERROR_CODE EnterUser(User* user); 
	void LeaveUser(User* user);
	
	int GetNumber() const				{ return m_number; }
	size_t GetCurrentUserCount() const	{ return m_user_table.size(); }

private:
	int m_number;
	std::unordered_map<std::string, User*> m_user_table;
};
