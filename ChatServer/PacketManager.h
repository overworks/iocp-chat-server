#pragma once

class UserManager;
class RoomManager;
class RedisManager;

class PacketManager
{
public:
	PacketManager();
	PacketManager(const PacketManager&) = delete;
	~PacketManager();

private:
	UserManager* m_user_manager;
	RoomManager* m_room_manager;
	RedisManager* m_redis_manager;
};