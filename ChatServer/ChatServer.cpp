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
ChatServer::OnInit()
{
	return true;
}

bool
ChatServer::OnRun()
{
	return true;
}

void
ChatServer::OnShutdown()
{
}


void
ChatServer::OnConnect(int index)
{

}

void
ChatServer::OnReceive(int index, size_t data_size, const char* data)
{

}

void
ChatServer::OnClose(int index)
{

}