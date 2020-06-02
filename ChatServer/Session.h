#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <queue>
#include <mutex>
#include <chrono>
#include <winsock2.h>

enum class IOOperation
{
	ACCEPT,
	RECEIVE,
	SEND,
};

struct OverlappedEx : public OVERLAPPED
{
	WSABUF WSABuf;
	IOOperation Operation;
	int SessionIndex;
};

constexpr size_t MAX_SOCK_RECVBUF = 256;

class Session
{
public:
	Session(int index, HANDLE iocp_handle);

	int GetIndex() const { return m_index; }
	SOCKET GetSocket() const { return m_socket; }
	bool IsConnected() const { return m_connected; }

	bool PostAccept(SOCKET listen_socket);
	bool AcceptCompletion();
	void Clear();
	void Close(bool force = false);

	bool OnConnect(); 
	
	bool BindRecv();

	void SendMsg(size_t data_size, const char* msg);
	void OnSendCompleted(size_t data_size);

	const char* RecvBuffer() const { return m_receive_buffer; }

	std::chrono::steady_clock::time_point GetLatestClosedTime() const { return m_latest_closed_time; }

private:
	bool BindIOCopmpletion();
	bool SendIO();

private:
	int m_index;
	HANDLE m_iocp_handle;
	SOCKET m_socket;

	bool m_connected;
	std::chrono::steady_clock::time_point m_latest_closed_time;

	char m_accept_buffer[64];
	OverlappedEx m_accept_context;
	
	char m_receive_buffer[MAX_SOCK_RECVBUF];
	OverlappedEx m_receive_context;

	std::mutex m_send_data_mutex;
	std::queue<OverlappedEx*> m_send_data_queue;
};
