#include "Packet.h"
#include "User.h"
#include "UserManager.h"
#include "Room.h"
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

	RegisterPacket();
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
PacketManager::ReceivePacketData(int session_index, size_t data_size, const char * data)
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
		if (m_incoming_packet_user_queue.empty())
		{
			return nullptr;
		}

		session_index = m_incoming_packet_user_queue.front();
		m_incoming_packet_user_queue.pop();
	}

	auto user = m_user_manager->GetUserByConnIdx(session_index);
	auto packet = user->GetPacket();

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
			idle = false;
			ProcessRecvPacket(packet->session_index, packet->packet_id, packet->length, packet->data);
		}

		packet = DequeueSystemPacket();
		if (packet != nullptr && packet->packet_id != PacketID::NONE)
		{
			idle = false;
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

	if (m_user_manager->IsFull())
	{
		//접속자수가 최대수를 차지해서 접속불가
		PacketLoginRes res_packet(ERROR_CODE::LOGIN_USER_USED_ALL_OBJ);
		m_send_func(session_index, res_packet.length, reinterpret_cast<const char*>(&res_packet));
		return;
	}

	if (m_user_manager->FindUserIndexByID(user_id) != -1)
	{
		//접속중인 유저여서 실패를 반환한다.
		PacketLoginRes res_packet(ERROR_CODE::LOGIN_USER_ALREADY);
		m_send_func(session_index, res_packet.length, reinterpret_cast<const char*>(&res_packet));
		return;
	}
	else
	{
		m_user_manager->AddUser(user_id, static_cast<int>(session_index));

		PacketLoginRes res_packet(ERROR_CODE::NONE);
		m_send_func(session_index, res_packet.length, reinterpret_cast<const char*>(&res_packet));

		std::cout << "Log In success - " << std::endl;
		/*RedisLoginReq dbReq;
		memcpy(dbReq.UserID, pLoginReqPacket->UserID, (MAX_USER_ID_LEN + 1));
		memcpy(dbReq.UserPW, pLoginReqPacket->UserPW, (MAX_USER_PW_LEN + 1));

		RedisTaskPtr task = std::make_shared<RedisTask>();
		task->UserIndex = clientIndex;
		task->TaskID = RedisTaskID::REQUEST_LOGIN;
		task->DataSize = sizeof(RedisLoginReq);
		task->pData = new char[task->DataSize];
		memcpy(task->pData, &dbReq, task->DataSize);
		mRedisMgr->PushRequest(task);

		mUserManager->AddUser(userID, static_cast<int>(clientIndex));

		std::cout << "Login To Redis user id = " << userID << std::endl;*/
	}
}

void
PacketManager::ProcessEnterRoom(Mh::u32 session_index, Mh::u16 packet_size, const char * packet_data)
{
	std::cout << "[ProcessEnterRoom] session_index: " << session_index << std::endl;

	auto user = m_user_manager->GetUserByConnIdx(session_index);
	if (!user)
	{
		return;
	}

	auto req_packet = reinterpret_cast<const PacketRoomEnterReq*>(packet_data);
	ERROR_CODE result = m_room_manager->EnterUser(req_packet->room_number, user);
	PacketRoomEnterRes res_packet(result);
	m_send_func(session_index, res_packet.length, reinterpret_cast<const char*>(&res_packet));
}

void
PacketManager::ProcessLeaveRoom(Mh::u32 session_index, Mh::u16 packet_size, const char * packet_data)
{
	std::cout << "[ProcessLeaveRoom] session_index: " << session_index << std::endl;

	auto user = m_user_manager->GetUserByConnIdx(session_index);
	if (!user)
	{
		return;
	}

	auto req_packet = reinterpret_cast<const PacketRoomLeaveReq*>(packet_data);
	ERROR_CODE result = m_room_manager->LeaveUser(user);
	
	//TODO Room안의 UserList객체의 값 확인하기
	PacketRoomLeaveRes res_packet(result);
	m_send_func(session_index, res_packet.length, reinterpret_cast<const char*>(&res_packet));
}

void
PacketManager::ProcessChatMessage(Mh::u32 session_index, Mh::u16 packet_size, const char * packet_data)
{
	std::cout << "[ProcessChatMessage] session_index: " << session_index << std::endl;

	User* user = m_user_manager->GetUserByConnIdx(session_index);
	if (!user)
	{
		std::cerr << "Invalid session index - " << session_index << std::endl;
		
		return;
	}

	int room_number = user->GetRoomNumber();
	if (room_number == -1)
	{
		std::cerr << "Invalid room number - " << room_number << std::endl;
		PacketRoomChatRes res_packet(ERROR_CODE::CHAT_ROOM_INVALID_ROOM_NUMBER);
		return;
	}

	Room* room = m_room_manager->GetRoom(room_number);
	if (!room)
	{
		std::cerr << "Invalid room number - " << room_number << std::endl;
		PacketRoomChatRes res_packet(ERROR_CODE::CHAT_ROOM_INVALID_ROOM_NUMBER);
		return;
	}

	auto req_packet = reinterpret_cast<const PacketRoomChatReq*>(packet_data);
	
	PacketRoomChatRes res_packet(ERROR_CODE::NONE);
	m_send_func(session_index, res_packet.length, reinterpret_cast<const char*>(&res_packet));

	// 방안의 사람에게 전달.
	PacketRoomNotify notify_packet(user->GetUserID().c_str(), req_packet->message);
	
	auto user_table = room->GetUserTable();
	for (auto iter : user_table)
	{
		auto dest_user = iter.second;
		if (!dest_user)
		{
			continue;
		}

		if (dest_user->GetIndex() == session_index)
		{
			continue;
		}

		m_send_func(dest_user->GetIndex(), notify_packet.length, reinterpret_cast<const char*>(&notify_packet));
	}
}
