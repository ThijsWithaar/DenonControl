#include "dummyConnection.h"


DummyConnection::DummyConnection(std::string_view data):
	m_data(data),
	m_offset(0),
	m_parser(Denon::Http::Method::Get)
{
}


size_t DummyConnection::read(char* dst, size_t nBuf)
{
	size_t nRem = m_data.size() - m_offset;
	size_t nRead = std::min(nBuf, nRem);
	std::memcpy(dst, &m_data[m_offset], nRead);
	m_offset += nRead;
	return nRead;
}

const Denon::Http::Response& DummyConnection::Http(const Denon::Http::Request& req)
{
	auto buf = m_parser.currentBuffer();
	size_t nRead = read(buf.first, buf.second);
	if(auto pPacket = m_parser.markRead(nRead))
	{
		m_response = *pPacket;
	}
	return m_response;
}
