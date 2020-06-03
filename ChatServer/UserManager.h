#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "ErrorCode.h"

class User;

class UserManager
{
public:
	UserManager() = default;
	UserManager(const UserManager&) = delete;
	~UserManager();

	void Init(size_t max_user_count);
	void Release();

	ERROR_CODE AddUser(const std::string& user_id, int client_index);
	int FindUserIndexByID(const std::string& user_id);
	void DeleteUserInfo(User* user);

	User* GetUserByConnIdx(int client_index);

	bool IsFull() const;

private:
	std::vector<User*> m_user_pool;
	std::unordered_map<std::string, int> m_user_id_table;
};