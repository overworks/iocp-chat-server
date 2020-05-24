#pragma once

#include "IOCPServer.h"

class ChatServer : public IOCPServer
{
public:
	virtual ~ChatServer();

protected:
	virtual bool OnInit() override;
	virtual bool OnRun() override;
	virtual void OnShutdown() override;

	virtual void OnConnect(int index) override;
	virtual void OnReceive(int index, size_t data_size, const char* data) override;
	virtual void OnClose(int index) override;
};