#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <vector>
#include <thread>
#include <memory>
#include <winsock2.h>
#include "Types.h"

class Session;

class IOCPServer
{
public:
	IOCPServer();
	virtual ~IOCPServer();

	bool Init(Mh::u16 port, Mh::u32 concurrent_threads_count, Mh::u32 session_count);
	bool Run(Mh::u32 client_count);
	void Shutdown();

protected:
	virtual bool OnInit(size_t session_count) = 0;
	virtual bool OnRun() = 0;
	virtual void OnShutdown() = 0;

	virtual void OnConnect(int index) = 0;
	virtual void OnClose(int index) = 0;
	virtual void OnReceive(int index, size_t data_size, const char* data) = 0;

	Mh::u32 GetConcurrentThreadsCount() const { return m_concurrent_threads_count; }
	Session* GetClient(int index) { return m_clients[index]; }

	void SendMsg(int session_index, Mh::u32 length, const char* data);

private:
	bool InitListenSocket(Mh::u16 port);
	void CloseListenSocket();

	bool InitIocp(Mh::u32 concurrent_threads_count);
	void CloseIocp();
	
	void CreateClientList(Mh::u32 client_count);
	void ClearClientList();

	void StartCommonThreads();
	void AccepterThreadProcess();
	void WorkerThreadProcess();
	void FinalizeCommonThreads();

	void CloseSocket(Session* client, bool force = false);

private:
	Mh::u32 m_concurrent_threads_count;

	SOCKET m_listen_socket;
	HANDLE m_iocp_handle;

	std::vector<Session*> m_clients;

	bool m_accepter_running;
	std::thread m_accepter_thread;
	bool m_worker_running;
	std::vector<std::thread> m_worker_threads;
};
