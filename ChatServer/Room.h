#pragma once

#include <unordered_map>
#include <functional>
#include "ErrorCode.h"

class User;

class Room
{
public:
	using UserTable = std::unordered_map<std::string, User*>;

public:
	Room(int number);
	Room(const Room&) = delete;
	~Room() = default;

	ERROR_CODE EnterUser(User* user); 
	void LeaveUser(User* user);

	int GetNumber() const				{ return m_number; }
	size_t GetCurrentUserCount() const	{ return m_user_table.size(); }

	const UserTable& GetUserTable() const { return m_user_table; }

private:
	int m_number;
	UserTable m_user_table;
};
