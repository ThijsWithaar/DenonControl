#include <Denon/network/ssdpIpc.h>

#include <filesystem>

#define DEBUG_LVL 2
#ifdef DEBUG_LVL
	#include <iostream>
#endif


namespace MiniSsdp {



template<typename BufferIterator>
size_t decodeMinisspdLen(BufferIterator pData, size_t& idx)
{
	size_t szStr = 0;
	do
	{
		szStr = (szStr << 7) | (pData[idx] & 0x7f);
	}
	while(pData[idx++] & 0x80);

	return szStr;
}


void AddCodelength(std::vector<char>& dst, size_t n)
{
	if(n>=268435456)
		dst.push_back(static_cast<char>(n >> 28) | 0x80);
	if(n>=2097152)
		dst.push_back(static_cast<char>(n >> 21) | 0x80);
	if(n>=16384)
		dst.push_back(static_cast<char>(n >> 14) | 0x80);
	if(n>=128)
		dst.push_back(static_cast<char>(n >> 7) | 0x80);
	dst.push_back(static_cast<char>(n & 0x7f));
}


void AddString(std::vector<char>& dst, std::string_view str)
{
	#if DEBUG_LVL > 1
		std::cout << "MiniIpcClient::AddString : " << str << "\n";
	#endif
	AddCodelength(dst, str.size());
	std::copy(str.begin(), str.end(), std::back_inserter(dst));
}


//-- Server --


Server::Settings::Settings():
	#ifdef __linux__
		socketName("/var/run/minissdpd.sock")
	#else
		socketName("\\\\.\\pipe\\minissdpd")
	#endif
{
}


Server::Server(
		boost::asio::io_context& context,
		Ssdp::ServiceCache& cache,
		Settings s):
	#ifdef __linux__
		m_acceptor(context),
	#endif
	m_socket(context),
	m_cache(cache)
{
	if(!s.socketName.empty())
	{
		#ifdef __linux__
			std::filesystem::remove(s.socketName);
			m_acceptor = {context, boost::asio::local::stream_protocol::endpoint(s.socketName)};
		#else
		DWORD dwOpenMode = PIPE_ACCESS_DUPLEX;
		DWORD dwPipeMode = PIPE_TYPE_MESSAGE;
		DWORD nMaxInstances = 1;
		DWORD nOutBufferSize = 1 << 10;
		DWORD nInBufferSize = 1 << 10;
		DWORD nDefaultTimeOut = 0;
		LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr;
			HANDLE handle = ::CreateNamedPipeA(
				s.socketName.c_str(),
				dwOpenMode,
				dwPipeMode,
				nMaxInstances,
				nOutBufferSize,
				nInBufferSize,
				nDefaultTimeOut,
				lpSecurityAttributes
			);
			m_socket = {context, handle };
		#endif
		#if DEBUG_LVL > 0
			std::cout << "SSDP IPC on " << s.socketName << "\n";
		#endif
	}
	ScheduleAccept();
}


void Server::ScheduleAccept()
{
#ifdef __linux__
	m_acceptor.async_accept(m_socket,
		[&](auto err)
		{
			#if DEBUG_LVL > 1
				std::cout << "MiniIPC: Accepted connection\n";
			#endif
			ScheduleRx();
			ScheduleAccept();
		}
	);
#endif
}


void Server::ScheduleRx()
{
	boost::asio::streambuf::mutable_buffers_type mutableBuffer = m_rxBuf.prepare(64);
#ifdef __linux__
	m_socket.async_receive(
		boost::asio::buffer(mutableBuffer),
		[&](const boost::system::error_code& error, size_t bytes_recvd)
		{
			// https://miniupnp.tuxfamily.org/minissdpd.html
			auto pData = boost::asio::buffers_begin(m_rxBuf.data());

			RequestType requestType = static_cast<RequestType>(pData[0]);
			switch(requestType)
			{
			case RequestType::GetVersion:
				GetVersion(); break;
			case RequestType::Type:
				GetType(); break;
			case RequestType::Usn:
				GetUSN(); break;
			case RequestType::All:
				GetAll(); break;
			default:
				break;
			}
		}
	);
#endif
}


std::string_view Server::ReadString()
{
	std::string_view msg;
	auto completion = [this, &msg](auto err, size_t nrTransferred) -> size_t
	{
		if(nrTransferred < 1)
			return 1;

		auto pData = boost::asio::buffers_begin(m_rxBuf.data());
		size_t idx = 1;
		for(; idx < nrTransferred; idx++)
			if((pData[idx] & 0x80) == 0)
				break;
		if(idx < nrTransferred)
			return 1;

		idx = 1;
		size_t strLen = decodeMinisspdLen(pData, idx);
		msg = {&pData[idx], strLen};

		return nrTransferred - strLen - 1;
	};

	size_t nRead = read(m_socket, m_rxBuf, completion);
	m_rxBuf.consume(nRead);

	#if DEBUG_LVL > 0
		std::cout << "MiniIPC: Received request: " << msg << "\n";
	#endif

	return msg;
}


void Server::GetIf(std::function<bool(const Ssdp::Notify&)> condition)
{
	m_txBuf.clear();
	m_txBuf.push_back(0);

	char& nrServices = m_txBuf[0];
	for(auto& it: m_cache.services)
	{
		if(!condition(it.second))
			continue;
		AddString(m_txBuf, it.second.location);
		AddString(m_txBuf, it.second.nt);
		AddString(m_txBuf, it.second.custom["USN"]);
		nrServices++;
	}
#ifdef __linux__
	m_socket.send(boost::asio::buffer(m_txBuf));
#endif
}


void Server::GetVersion()
{
	m_txBuf.clear();
	AddString(m_txBuf, "1.5");
#ifdef __linux__
	m_socket.send(boost::asio::buffer(m_txBuf));
#endif
}


void Server::GetType()
{
	auto tp = ReadString();
	GetIf([&tp](const Ssdp::Notify& n)
	{
		return n.nt == tp;
	});
}


void Server::GetUSN()
{
	auto usn = ReadString();
	GetIf([&usn](const Ssdp::Notify& n)
	{
		return n.custom.count("USN") && n.custom.at("USN") == usn;
	});
}


void Server::GetAll()
{
	GetIf([](const Ssdp::Notify&)
	{
		return true;
	});
}


//-- Client --


Client::Client(boost::asio::io_context& ioc, Settings s):
	m_socket(ioc),
	m_settings(s)
{
#ifdef __linux__
	m_socket.connect(m_settings.socketName);
#endif
}


std::string Client::GetVersion()
{
	m_txBuf.clear();
	m_txBuf.push_back(static_cast<char>(Server::RequestType::GetVersion));

	auto r = Dispatch(Server::RequestType::GetVersion);
	if(!r.empty())
		return std::string(r[0]);
	return "";
}


std::vector<Ssdp::Response> Client::SearchUsn(std::string usn)
{
	#if DEBUG_LVL > 0
		std::cout << "MiniIpcClient::SearchUsn\n";
	#endif

	m_txBuf.clear();
	m_txBuf.push_back(static_cast<char>(Server::RequestType::Usn));
	AddString(m_txBuf, usn);

	return SplitResponses(Dispatch(Server::RequestType::Usn));
}


std::vector<Ssdp::Response> Client::SearchType(std::string ssdpType)
{
	#if DEBUG_LVL > 0
		std::cout << "MiniIpcClient::SearchType\n";
	#endif

	m_txBuf.clear();
	m_txBuf.push_back(static_cast<char>(Server::RequestType::Type));
	AddString(m_txBuf, ssdpType);

	return SplitResponses(Dispatch(Server::RequestType::Type));
}


std::vector<Ssdp::Response> Client::RequestAll()
{
	#if DEBUG_LVL > 0
		std::cout << "MiniIpcClient::RequestAll\n";
	#endif

	m_txBuf.clear();
	m_txBuf.push_back(static_cast<char>(Server::RequestType::All));

	return SplitResponses(Dispatch(Server::RequestType::All));
}


void Client::Submit(Ssdp::Notify nt)
{
	m_txBuf.clear();
	m_txBuf.push_back(static_cast<char>(Server::RequestType::Submit));

	AddString(m_txBuf, nt.nt);
	AddString(m_txBuf, nt.custom["USN"]);
	AddString(m_txBuf, nt.custom["SERVER"]);
	AddString(m_txBuf, nt.location);
#ifdef __linux__
	m_socket.send(boost::asio::buffer(m_txBuf));
#endif
}


std::vector<std::string_view> Client::Dispatch(Server::RequestType rqType)
{
	using namespace boost::asio;

#ifdef __linux__
	size_t nSend = m_socket.send(boost::asio::buffer(m_txBuf));
	#if DEBUG_LVL > 0
		std::cout << "MiniIpcClient Send : " << nSend << " bytes\n";
	#endif
#endif

	// Protocol by design has a very complex completion criterion.
	// Alternative is to have a big buffer and issue a single socket read
	std::vector<std::string_view> strings;
	auto completion = [this, &strings, rqType](auto err, size_t nrTransferred) -> size_t
	{
		if(nrTransferred < 1)
			return 1;
		const auto pData = boost::asio::buffers_begin(m_rxBuf.data());
		int nrResponses = 1;
		int nrStrings = 1;
		size_t idx = 0;
		switch(rqType)
		{
			case Server::RequestType::GetVersion:
				nrResponses = nrStrings = 1;
				idx = 0;
				break;
			case Server::RequestType::Type:
			case Server::RequestType::Usn:
			case Server::RequestType::All:
				nrResponses = pData[0];
				nrStrings = nrResponses * 3;
				idx = 1;
				break;
			case Server::RequestType::Submit:
			default:
				nrResponses = nrStrings = 0;
		}
		#if DEBUG_LVL > 2
			std::cout << "Completion: expect " << nrResponses << " responses, " << nrString " strings\n";
		#endif

		if(nrStrings < 1)
			return 0;

		size_t nStr;
		strings.clear();
		for(nStr=0; idx < m_rxBuf.data().size() && nStr < nrStrings; nStr++)
		{
			size_t szStr = decodeMinisspdLen(pData, idx);
			#if DEBUG_LVL > 2
				std::cout << "Completion: string " << nStr << " of size " << szStr << "\n";
			#endif
			strings.push_back(std::string_view{&pData[idx], szStr});
			idx += szStr;
		}
		if(nStr < nrStrings)
			idx++;

		return idx - std::min(idx, nrTransferred);
	};

	size_t nRead = read(m_socket, m_rxBuf, completion);
	m_rxBuf.consume(nRead);

	#if DEBUG_LVL > 1
		for(auto st: strings)
			std::cout << "\tDispatch: " << st << "\n";
	#endif
	return strings;
}


std::vector<Ssdp::Response> Client::SplitResponses(std::vector<std::string_view> strings)
{
	std::vector<Ssdp::Response> ret;
	ret.reserve(strings.size()/3);
	for(ptrdiff_t n=0; n < static_cast<ptrdiff_t>(strings.size()) - 2; n+=3)
	{
		Ssdp::Response r;
		r.location = strings[n];
		r.nt = strings[n+1];
		r.custom["USN"] = strings[n+2];
		ret.push_back(r);
	}
	return ret;
}


} // namespace Ssdp
