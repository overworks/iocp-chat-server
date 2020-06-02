#include "User.h"
#include "UserManager.h"

UserManager::~UserManager()
{
	Release();
}

void
UserManager::Init(size_t max_user_count)
{
	m_user_pool.reserve(max_user_count);
	for (int i = 0, imax = static_cast<int>(max_user_count); i < imax; ++i)
	{
		m_user_pool.push_back(new User(i));
	}
}

void
UserManager::Release()
{
	for (auto user : m_user_pool)
	{
		delete user;
	}
	m_user_pool.clear();
}

ERROR_CODE
UserManager::AddUser(const std::string& user_id, int client_index)
{
	if (m_user_id_table.find(user_id) != m_user_id_table.end())
	{
		return ERROR_CODE::LOGIN_USER_ALREADY;
	}

	m_user_pool[client_index]->Login(user_id);
	m_user_id_table[user_id] = client_index;

	return ERROR_CODE::NONE;
}

int
UserManager::FindUserIndexByID(const std::string& user_id)
{
	auto iter = m_user_id_table.find(user_id);
	return iter != m_user_id_table.end() ? iter->second : -1;
}

void
UserManager::DeleteUserInfo(User* user)
{
	m_user_id_table.erase(user->GetUserID());
	user->Reset();
}

User*
UserManager::GetUserByConnIdx(int client_index)
{
	if (client_index < 0 || client_index >= static_cast<int>(m_user_pool.size()))
	{
		return nullptr;
	}
	return m_user_pool[static_cast<size_t>(client_index)];
}