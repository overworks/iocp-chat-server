#include "User.h"

User::User(int index)
	: m_index(index)
{
	Reset();
}

User::~User()
{
}

void
User::Reset()
{
	m_room_number = -1;
	
	m_user_id.clear();
	m_auth_token.clear(); 
	
	m_buffer_write_pos = 0;
	m_buffer_read_pos = 0;
}

void
User::Login(const std::string& user_id)
{
	m_user_id = user_id;
}

void
User::EnterRoom(int room_number)
{
	m_room_number = room_number;
}

void
User::LeaveRoom()
{
	m_room_number = -1;
}

void
User::SetPacketData(size_t data_size, const char* data)
{
	if (m_buffer_write_pos + data_size >= PACKET_DATA_BUFFER_SIZE)
	{
		// 아직 처리가 되지 않은 데이터가 있는지 확인
		size_t remain_data_size = m_buffer_write_pos - m_buffer_read_pos;
		if (remain_data_size > 0)
		{
			// 처리가 되지 않은 데이터를 앞으로 끌어당김.
			// TODO 이렇게 해도 넘칠 위험이 있으므로 체크해줘야 함.
			memcpy(m_buffer, m_buffer + m_buffer_read_pos, remain_data_size);
			m_buffer_read_pos = 0;
			m_buffer_write_pos = remain_data_size;
		}
		else
		{
			m_buffer_read_pos = 0;
			m_buffer_write_pos = 0;
		}
	}

	memcpy(m_buffer + m_buffer_write_pos, data, data_size);
	m_buffer_write_pos += data_size;
}

PacketInfoPtr
User::GetPacket()
{
	auto unsended_bytes = m_buffer_write_pos - m_buffer_read_pos;
	if (unsended_bytes < PACKET_HEADER_LENGTH)
	{
		return nullptr;
	}

	auto header = reinterpret_cast<PacketHeader*>(m_buffer + m_buffer_read_pos);
	if (header->length > unsended_bytes)
	{
		return nullptr;
	}

	PacketInfoPtr packet = std::make_shared<PacketInfo>();
	packet->session_index = m_index;
	packet->packet_id = header->packet_id;
	packet->length = header->length;
	packet->data = m_buffer + m_buffer_read_pos;

	m_buffer_read_pos += header->length;

	return packet;
}
