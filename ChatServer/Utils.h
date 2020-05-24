#pragma once

#include <thread>

inline void JoinThread(std::thread& thread)
{
	if (thread.joinable())
	{
		thread.join();
	}
}