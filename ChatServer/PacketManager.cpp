#include "Packet.h"
#include "User.h"
#include "UserManager.h"
#include "RoomManager.h"
#include "PacketManager.h"
#include "Utils.h"
#include <iostream>

PacketManager::PacketManager()
{
	m_user_manager = new UserManager();
	m_room_manager = new RoomManager();
}

PacketManager::~PacketManager()
{
	Release();
}

void PacketManager::Init(size_t count, PacketManager::SendPacketFunc send_func)
{
	m_send_func = send_func;

	m_user_manager->Init(count);
	m_room_manager->Init(count, MaxRoomCount, BeginRoomNumber);
}

void PacketManager::Run()
{
	m_process_running = true;
	m_process_thread = std::thread([this]() { ProcessPacket(); });
}

void PacketManager::Stop()
{
	m_process_running = false;
	JoinThread(m_process_thread);
}

void
PacketManager::ReceivePacketData(int session_index, Mh::u32 data_size, const char * data)
{
	auto user = m_user_manager->GetUserByConnIdx(session_index);
	user->SetPacketData(data_size, data);
	
	EnqueueUserPacket(session_index);
}

void
PacketManager::PushSystemPacket(const PacketInfoPtr& packet)
{
	std::lock_guard<std::mutex> guard(m_lock);
	m_system_packet_queue.push(packet);
}

void PacketManager::RegisterPacket()
{
	m_recv_func_table.clear();

	m_recv_func_table[PacketID::SYS_USER_CONNECT] = &PacketManager::ProcessUserConnect;
	m_recv_func_table[PacketID::SYS_USER_DISCONNECT] = &PacketManager::ProcessUserDisconnect;
	
	m_recv_func_table[PacketID::LOGIN_REQUEST] = &PacketManager::ProcessLogin;
	m_recv_func_table[PacketID::ROOM_ENTER_REQUEST] = &PacketManager::ProcessEnterRoom;
	m_recv_func_table[PacketID::ROOM_LEAVE_REQUEST] = &PacketManager::ProcessLeaveRoom;
	m_recv_func_table[PacketID::ROOM_CHAT_REQUEST] = &PacketManager::ProcessChatMessage;
}

void PacketManager::Release()
{
	if (m_user_manager)
	{
		delete m_user_manager;
		m_user_manager = nullptr;
	}

	if (m_room_manager)
	{
		delete m_room_manager;
		m_room_manager = nullptr;
	}
}

void
PacketManager::ClearConnectionInfo(int session_index)
{
	auto user = m_user_manager->GetUserByConnIdx(session_index);

	if (user->IsInRoom())
	{
		m_room_manager->LeaveUser(user);
	}

	if (!user->IsLoggedIn())
	{
		m_user_manager->DeleteUserInfo(user);
	}
}

void
PacketManager::EnqueueUserPacket(int session_index)
{
	std::lock_guard<std::mutex> guard(m_lock);
	m_incoming_packet_user_queue.push(session_index);
}

PacketInfoPtr
PacketManager::DequeueSystemPacket()
{
	std::lock_guard<std::mutex> guard(m_lock);
	if (m_system_packet_queue.empty())
	{
		return nullptr;
	}

	auto packet = m_system_packet_queue.front();
	m_system_packet_queue.pop();

	return packet;
}

PacketInfoPtr
PacketManager::DequeueUserPacket()
{
	int session_index;
	{
		std::lock_guard<std::mutex> guard(m_lock);
		if (!m_incoming_packet_user_queue.empty())
		{
			return nullptr;
		}

		session_index = m_incoming_packet_user_queue.front();
		m_incoming_packet_user_queue.pop();
	}

	auto user = m_user_manager->GetUserByConnIdx(session_index);
	auto packet = user->GetPacket();
	packet->session_index = session_index;

	return packet;
}

void
PacketManager::ProcessPacket()
{
	bool idle;
	while (m_process_running)
	{
		idle = true;

		auto packet = DequeueUserPacket();
		if (packet != nullptr && packet->packet_id > PacketID::SYS_END)
		{
			idle = true;
			ProcessRecvPacket(packet->session_index, packet->packet_id, packet->length, packet->data);
		}

		packet = DequeueSystemPacket();
		if (packet != nullptr && packet->packet_id != PacketID::NONE)
		{
			idle = true;
			ProcessRecvPacket(packet->session_index, packet->packet_id, packet->length, packet->data);
		}

		if (idle)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void
PacketManager::ProcessRecvPacket(Mh::u32 session_index, PacketID packet_id, Mh::u16 packet_size, const char * packet_data)
{
	auto iter = m_recv_func_table.find(packet_id);
	if (iter != m_recv_func_table.end())
	{
		(this->*(iter->second))(session_index, packet_size, packet_data);
	}
}

void
PacketManager::ProcessUserConnect(Mh::u32 session_index, Mh::u16 packet_size, const char * packet)
{
	std::cout << "[ProcessUserConnect] clientIndex: " << session_index << std::endl;
	auto user = m_user_manager->GetUserByConnIdx(session_index);
	user->Reset();
}

void
PacketManager::ProcessUserDisconnect(Mh::u32 session_index, Mh::u16 packet_size, const char * packet)
{
	std::cout << "[ProcessUserDisconnect] session_index: " << session_index << std::endl;
	ClearConnectionInfo(static_cast<int>(session_index));
}

void
PacketManager::ProcessLogin(Mh::u32 session_index, Mh::u16 packet_size, const char * packet_data)
{
	std::cout << "[ProcessUserLogin] session_index: " << session_index << std::endl;
	
	auto req_packet = reinterpret_cast<const PacketLoginReq*>(packet_data);
	
	std::string user_id(req_packet->user_id);
	std::cout << "requested user id = " << user_id << std::endl;

	PacketLoginRes packet_res;
}

void
PacketManager::ProcessEnterRoom(Mh::u32 session_index, Mh::u16 packet_size, const char * packet)
{
}

void
PacketManager::ProcessLeaveRoom(Mh::u32 session_index, Mh::u16 packet_size, const char * packet)
{
}

void
PacketManager::ProcessChatMessage(Mh::u32 session_index, Mh::u16 packet_size, const char * packet)
{
}
