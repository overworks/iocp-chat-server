#pragma once

#define WIN32_LEAN_AND_MEAN

#include "Types.h"

#include <functional>

using SendPacketFunc = std::function<void(uint32_t, uint32_t, const char*)>;

enum class PacketID : Mh::u16
{
	None
};

struct PacketHeader
{
	PacketID packet_id;
	Mh::u16 length;
};

constexpr size_t PACKET_HEADER_LENGTH = sizeof(PacketHeader);