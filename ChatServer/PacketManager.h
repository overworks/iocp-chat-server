#pragma once

#include "Packet.h"
#include <queue>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>

class UserManager;
class RoomManager;
class RedisManager;

constexpr int BeginRoomNumber = 0;
constexpr size_t MaxRoomCount = 10;
constexpr size_t MaxRoomUserCount = 4;

class PacketManager
{
public:
	using SendPacketFunc = std::function<void(uint32_t, uint32_t, const char*)>;

public:
	PacketManager();
	PacketManager(const PacketManager&) = delete;
	~PacketManager();

	void Init(size_t max_count, SendPacketFunc send_func);
	void Run();
	void Stop();

	void ReceivePacketData(int session_index, Mh::u32 data_size, const char* data);
	void PushSystemPacket(const PacketInfoPtr& packet);

private:
	void RegisterPacket();
	void Release();

	void ClearConnectionInfo(int session_index); 
	
	void EnqueueUserPacket(int session_index);
	
	PacketInfoPtr DequeueSystemPacket();
	PacketInfoPtr DequeueUserPacket();

	void ProcessPacket();
	void ProcessRecvPacket(Mh::u32 session_index, PacketID packet_id, Mh::u16 packet_size, const char* packet_data);

	void ProcessUserConnect(Mh::u32 session_index, Mh::u16 packet_size, const char* packet);
	void ProcessUserDisconnect(Mh::u32 session_index, Mh::u16 packet_size, const char* packet);

	void ProcessLogin(Mh::u32 session_index, Mh::u16 packet_size, const char* packet);
	void ProcessEnterRoom(Mh::u32 session_index, Mh::u16 packet_size, const char* packet);
	void ProcessLeaveRoom(Mh::u32 session_index, Mh::u16 packet_size, const char* packet);
	void ProcessChatMessage(Mh::u32 session_index, Mh::u16 packet_size, const char* packet);

private:
	typedef void(PacketManager::*PacketFunc)(uint32_t, uint16_t, const char*);

	UserManager* m_user_manager;
	RoomManager* m_room_manager;
	RedisManager* m_redis_manager;

	bool m_process_running;
	std::thread m_process_thread;
	std::mutex m_lock;

	std::unordered_map<PacketID, PacketFunc> m_recv_func_table;

	std::queue<int> m_incoming_packet_user_queue;
	std::queue<PacketInfoPtr> m_system_packet_queue;

	SendPacketFunc m_send_func;
};