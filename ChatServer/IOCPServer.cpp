#include <iostream>
#include <chrono>
#include <algorithm>
#include "Utils.h"
#include "Client.h"
#include "IOCPServer.h"

using namespace std::chrono;

IOCPServer::IOCPServer()
	: m_listen_socket(INVALID_SOCKET)
	, m_iocp_handle(nullptr)
{

}

IOCPServer::~IOCPServer()
{
	
}

bool
IOCPServer::Init(Mh::u16 port, Mh::u32 concurrent_threads_count)
{
	::WSADATA wsaData;

	if (0 != ::WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		std::cerr << u8"[에러] WSAStartup()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	if (!InitListenSocket(port))
	{
		return false;
	}

	if (!InitIocp(concurrent_threads_count))
	{
		return false;
	}

	m_concurrent_threads_count = concurrent_threads_count;

	if (!OnInit())
	{
		return false;
	}

	return true;
}

bool
IOCPServer::Run(Mh::u32 client_count)
{
	CreateClientList(client_count);
	StartCommonThreads();

	if (!OnRun())
	{
		return false;
	}

	return true;
}

void
IOCPServer::Shutdown()
{
	OnShutdown();

	FinalizeCommonThreads();
	ClearClientList();
	CloseIocp();
	CloseListenSocket();

	::WSACleanup();
}

bool IOCPServer::InitListenSocket(Mh::u16 port)
{
	//연결지향형 TCP , Overlapped I/O 소켓을 생성
	m_listen_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_listen_socket)
	{
		std::cerr << u8"[에러] socket()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	::SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = ::htons(port); //서버 포트를 설정한다.		
	//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
	//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
	//그 주소를 inet_addr함수를 이용해 넣으면 된다.
	addr.sin_addr.s_addr = ::htonl(INADDR_ANY);

	//위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
	if (0 != ::bind(m_listen_socket, reinterpret_cast<SOCKADDR*>(&addr), sizeof(SOCKADDR_IN)))
	{
		std::cerr << u8"[에러] bind()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	//접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
	//접속대기큐를 5개로 설정 한다.
	if (0 != ::listen(m_listen_socket, 5))
	{
		std::cerr << u8"[에러] listen()함수 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

bool IOCPServer::InitIocp(Mh::u32 concurrent_threads_count)
{
	//CompletionPort객체 생성 요청을 한다.
	m_iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent_threads_count);
	if (nullptr == m_iocp_handle)
	{
		std::cerr << u8"[에러] CreateIoCompletionPort()함수 실패: " << ::GetLastError() << std::endl;
		return false;
	}

	HANDLE hIOCPHandle = ::CreateIoCompletionPort((HANDLE)m_listen_socket, m_iocp_handle, 0, 0);
	if (nullptr == hIOCPHandle)
	{
		std::cerr << u8"[에러] listen socket IOCP bind 실패 : " << ::WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

void
IOCPServer::CloseIocp()
{
	if (m_iocp_handle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_iocp_handle);
		m_iocp_handle = INVALID_HANDLE_VALUE;
	}
}

void
IOCPServer::CreateClientList(Mh::u32 client_count)
{
	m_clients.reserve(client_count);
	
	for (auto i = 0u; i < client_count; ++i)
	{
		m_clients.push_back(new Client(static_cast<int>(i), m_iocp_handle));
	}
}

void
IOCPServer::ClearClientList()
{
	for (auto client : m_clients)
	{
		delete client;
	}
	m_clients.clear();
}

void
IOCPServer::CloseListenSocket()
{
	if (m_listen_socket != INVALID_SOCKET)
	{
		/*if (0 != ::shutdown(m_listen_socket, SD_BOTH))
		{
			std::cerr << u8"[에러] shutdown 실패" << ::WSAGetLastError() << std::endl;
		}*/

		::closesocket(m_listen_socket);
		m_listen_socket = INVALID_SOCKET;
	}
}

void
IOCPServer::StartCommonThreads()
{
	auto worker_thread_count = m_concurrent_threads_count * 2 + 1;
	m_worker_running = true;
	m_worker_threads.reserve(worker_thread_count);
	for (auto i = 0u; i < worker_thread_count; ++i)
	{
		m_worker_threads.push_back(std::thread([this]() { WorkerThreadProcess(); }));
	}
	std::cout << u8"Worker threads started." << std::endl;

	m_accepter_running = true;
	m_accepter_thread = std::thread([this]() { AccepterThreadProcess(); });
	std::cout << u8"Accepter thread started." << std::endl;
}

void
IOCPServer::AccepterThreadProcess()
{
	while (m_accepter_running)
	{
		auto now = std::chrono::steady_clock::now();
		for (auto client : m_clients)
		{
			if (client->IsConnected())
			{
				continue;
			}

			auto latest_closed_time = client->GetLatestClosedTime();
			auto diff_sec = duration_cast<seconds>(now - latest_closed_time);

			// 접속이 닫힌지 3초가 지나지 않으면 패스
			if (diff_sec <= seconds(3))
			{
				continue;
			}

			client->PostAccept(m_listen_socket);
		}

		std::this_thread::sleep_for(std::chrono::microseconds(32));
	}
}

void
IOCPServer::WorkerThreadProcess()
{
	Client* client;
	BOOL succeeded;
	DWORD num_transfered;
	LPOVERLAPPED overlapped;

	while (m_worker_running)
	{
		succeeded = ::GetQueuedCompletionStatus(m_iocp_handle,
			&num_transfered,
			reinterpret_cast<PULONG_PTR>(&client),
			&overlapped,
			INFINITE);

		// 사용자 스레드 종료 메시지
		if (succeeded && 0 == num_transfered && !overlapped)
		{
			m_worker_running = false;
			continue;
		}

		if (!overlapped)
		{
			continue;
		}

		OverlappedEx* overlapped_ex = static_cast<OverlappedEx*>(overlapped);

		//client가 접속을 끊었을때..			
		if (!succeeded || (0 == num_transfered && IOOperation::ACCEPT != overlapped_ex->Operation))
		{
			CloseSocket(client);
			continue;
		}

		if (IOOperation::ACCEPT == overlapped_ex->Operation)
		{
			client = GetClient(overlapped_ex->SessionIndex);
			if (client->AcceptCompletion())
			{
				OnConnect(client->GetIndex());
			}
			else
			{
				CloseSocket(client, true);  //Caller WokerThread()
			}
		}
		//Overlapped I/O Recv작업 결과 뒤 처리
		else if (IOOperation::RECEIVE == overlapped_ex->Operation)
		{
			OnReceive(client->GetIndex(), num_transfered, client->RecvBuffer());

			client->BindRecv();
		}
		//Overlapped I/O Send작업 결과 뒤 처리
		else if (IOOperation::SEND == overlapped_ex->Operation)
		{
			client->OnSendCompleted(num_transfered);
		}
		//예외 상황
		else
		{
			//printf(u8"Client Index(%d)에서 예외상황\n", client->GetIndex());
		}
	}
}

void
IOCPServer::FinalizeCommonThreads()
{
	m_worker_running = false;
	std::for_each(m_worker_threads.begin(), m_worker_threads.end(), JoinThread);

	m_accepter_running = false;
	JoinThread(m_accepter_thread);
}

void
IOCPServer::CloseSocket(Client * client, bool force)
{
	if (client->IsConnected())
	{
		client->Close(force);

		OnClose(client->GetIndex());
	}
}
