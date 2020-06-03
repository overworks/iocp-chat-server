#pragma once

#include "IOCPServer.h"

class PacketManager;

class ChatServer : public IOCPServer
{
public:
	ChatServer();
	virtual ~ChatServer();

protected:
	virtual bool OnInit(size_t session_count) override;
	virtual bool OnRun() override;
	virtual void OnShutdown() override;

	virtual void OnConnect(int index) override;
	virtual void OnClose(int index) override;
	virtual void OnReceive(int index, size_t data_size, const char* data) override;

private:
	PacketManager* m_packet_manager;
};