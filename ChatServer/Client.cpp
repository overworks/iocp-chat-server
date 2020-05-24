#define NOMINMAX

#include "Client.h"
#include <iostream>
#include <mswsock.h>
#include <ws2tcpip.h>

using namespace std::chrono;

Client::Client(int index, HANDLE iocp_handle)
	: m_index(index)
	, m_iocp_handle(iocp_handle)
	, m_socket(INVALID_SOCKET)
	, m_connected(false)
{
}

bool
Client::PostAccept(SOCKET listen_socket)
{
	std::cout << "[PostAccept] index - " << GetIndex() << std::endl;

	m_latest_closed_time = steady_clock::time_point::max();

	m_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_socket)
	{
		std::cerr << "[PostAccept] WSASocket error - " << ::GetLastError() << std::endl;
		return false;
	}

	memset(&m_accept_context, 0, sizeof(OverlappedEx));
	m_accept_context.Operation = IOOperation::ACCEPT;
	m_accept_context.SessionIndex = GetIndex();

	DWORD bytes;

	if (FALSE == ::AcceptEx(listen_socket, m_socket, m_accept_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, reinterpret_cast<LPWSAOVERLAPPED>(&m_accept_context)))
	{
		if (::WSAGetLastError() != WSA_IO_PENDING)
		{
			std::cerr << "AcceptEx Error : " << ::GetLastError() << std::endl;
			return false;
		}
	}

	return true;
}

bool
Client::AcceptCompletion()
{
	std::cout << "AcceptCompletion - SessionIndex: " << GetIndex() << std::endl;

	if (!OnConnect())
	{
		return false;
	}

	::SOCKADDR_IN addr;
	char ip[32] = { 0, };
	::inet_ntop(AF_INET, &addr.sin_addr, ip, 32 - 1);

	std::cout << u8"클라이언트 접속 - IP: " << ip << std::endl;

	return true;
}

void Client::Clear()
{
}

void
Client::Close(bool force)
{
	linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정

	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	if (force)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose소켓의 데이터 송수신을 모두 중단 시킨다.
	::shutdown(m_socket, SD_BOTH);

	//소켓 옵션을 설정한다.
	::setsockopt(m_socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&stLinger), sizeof(stLinger));

	m_connected = false;
	m_latest_closed_time = steady_clock::now();
	
	//소켓 연결을 종료 시킨다.
	::closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

void
Client::SendMsg(size_t data_size, const char * msg)
{
	OverlappedEx* overlapped_ex = new OverlappedEx();
	memset(overlapped_ex, 0, sizeof(OverlappedEx));
	overlapped_ex->WSABuf.len = static_cast<ULONG>(data_size);
	overlapped_ex->WSABuf.buf = new char[data_size];
	memcpy(overlapped_ex->WSABuf.buf, msg, data_size);
	overlapped_ex->Operation = IOOperation::SEND;

	{
		std::lock_guard<std::mutex> guard(m_send_data_mutex);
		m_send_data_queue.push(overlapped_ex);
	}

	if (!m_send_data_queue.empty())
	{
		SendIO();
	}
}

void
Client::OnSendCompleted(size_t data_size)
{
	std::cout << u8"[송신 완료] bytes : " << data_size << std::endl;

	OverlappedEx* overlapped_ex = nullptr;
	{
		std::lock_guard<std::mutex> guard(m_send_data_mutex);

		overlapped_ex = m_send_data_queue.front();
		m_send_data_queue.pop();
	}

	delete[] overlapped_ex->WSABuf.buf;
	delete overlapped_ex;

	if (!m_send_data_queue.empty())
	{
		SendIO();
	}
}

bool
Client::BindRecv()
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	m_receive_context.WSABuf.len = MAX_SOCK_RECVBUF;
	m_receive_context.WSABuf.buf = m_receive_buffer;
	m_receive_context.Operation = IOOperation::RECEIVE;

	int nRet = ::WSARecv(m_socket,
		&m_receive_context.WSABuf,
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED)&m_receive_context,
		nullptr);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cerr << u8"[에러] WSARecv()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
Client::OnConnect()
{
	m_connected = true;

	Clear();

	if (!BindIOCopmpletion())
	{
		return false;
	}

	return BindRecv();
}

bool Client::BindIOCopmpletion()
{
	//socket과 pClientInfo를 CompletionPort객체와 연결시킨다.
	auto hIOCP = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_socket),
		m_iocp_handle, reinterpret_cast<ULONG_PTR>(this), 0);
	if (hIOCP == INVALID_HANDLE_VALUE)
	{
		std::cerr << u8"[에러] CreateIoCompletionPort()함수 실패: " << ::GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool
Client::SendIO()
{
	auto overlapped_ex = m_send_data_queue.front();

	DWORD dwRecvNumBytes = 0;
	int nRet = ::WSASend(m_socket,
		&overlapped_ex->WSABuf,
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)overlapped_ex,
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (::WSAGetLastError() != ERROR_IO_PENDING))
	{
		std::cout << u8"[에러] WSASend()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}
