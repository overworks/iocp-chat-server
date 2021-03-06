﻿#include <iostream>
#include "ChatServer.h"
#include "PacketManager.h"

ChatServer::ChatServer()
{
	m_packet_manager = new PacketManager();
}

ChatServer::~ChatServer()
{
	delete m_packet_manager;
}

bool
ChatServer::OnInit(size_t session_count)
{
	auto send_func = [this](Mh::u32 session_index, Mh::u32 length, const char* data) {
		SendMsg(session_index, length, data);
	};

	m_packet_manager->Init(session_count, send_func);
	return true;
}

bool
ChatServer::OnRun()
{
	m_packet_manager->Run();
	return true;
}

void
ChatServer::OnShutdown()
{
}


void
ChatServer::OnConnect(int index)
{
	std::cout << "[OnConnect] session index: " << index << std::endl;

	auto packet = std::make_shared<PacketInfo>();
	packet->session_index = index;
	packet->packet_id = PacketID::SYS_USER_CONNECT;

	m_packet_manager->PushSystemPacket(packet);
}

void
ChatServer::OnClose(int index)
{
	std::cout << "[OnClose] session index: " << index << std::endl;

	auto packet = std::make_shared<PacketInfo>();
	packet->session_index = index;
	packet->packet_id = PacketID::SYS_USER_DISCONNECT;
	
	m_packet_manager->PushSystemPacket(packet);
}

void
ChatServer::OnReceive(int index, size_t data_size, const char* data)
{
	std::cout << "[OnReceive] session index: " << index << std::endl;

	m_packet_manager->ReceivePacketData(index, data_size, data);
}

