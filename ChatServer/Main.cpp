#include <string>
#include <iostream>
#include "ChatServer.h"

constexpr Mh::u16 SERVER_PORT = 11021;
constexpr Mh::u32 MAX_CLIENT = 3;		//총 접속할수 있는 클라이언트 수
constexpr Mh::u32 MAX_IO_WORKER_THREAD = 4;  //쓰레드 풀에 넣을 쓰레드 수

int main()
{
	SetConsoleOutputCP(CP_UTF8);

	ChatServer server;

	//소켓을 초기화
	server.Init(SERVER_PORT, MAX_IO_WORKER_THREAD);

	server.Run(MAX_CLIENT);

	std::cout << u8"아무 키나 누를 때까지 대기합니다" << std::endl;
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.Shutdown();
	return 0;
}

