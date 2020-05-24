#pragma once

#define WIN32_LEAN_AND_MEAN

#include "Types.h"

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