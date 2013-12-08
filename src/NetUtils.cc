#include "NetUtils.h"
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <cstring>

namespace NetUtils
{
	using namespace std;
	using namespace boost;
	//using namespace boost::interprocess;
	using namespace boost::asio;

	class udp_transceiver
	{
	public:
		udp_transceiver(io_service& service)
			:service(service)
		{
			socket = new ip::udp::socket(service, ip::udp::endpoint(ip::udp::v4(), 5678));

			socket->async_receive_from(boost::asio::buffer(recv_buffer), remote_endpoint_,
		        boost::bind(&udp_transceiver::handle_receive_from, this,
		        	boost::asio::placeholders::error,
		        	boost::asio::placeholders::bytes_transferred));
		}

		~udp_transceiver()
		{
			delete socket;
		}

	private:
		void handle_receive_from(const boost::system::error_code& error, std::size_t size)
		{
			recv_buffer[size >= 1024 ? 1023 : size] = 0;
			cout << "Packet received from " << remote_endpoint_.address().to_string() << ": " << recv_buffer.elems << endl;

			socket->async_receive_from(boost::asio::buffer(recv_buffer), remote_endpoint_,
		        boost::bind(&udp_transceiver::handle_receive_from, this,
		        	boost::asio::placeholders::error,
		        	boost::asio::placeholders::bytes_transferred));
		}

		boost::asio::io_service& service;
		ip::udp::socket *socket;

		ip::udp::endpoint remote_endpoint_;
		boost::array<char, 1024> recv_buffer;
	};

	class SizeTToken
	{
		uint32_t data;

	public:
		SizeTToken():data(0) {}

		SizeTToken(size_t size)
		{
			data = htonl((uint32_t)size);
		}

		size_t GetValue() const
		{
			return (size_t)ntohl(data);
		}

		asio::mutable_buffers_1 GetAsioBuffer()
		{
			return asio::buffer((void*) &data, sizeof(data));
		}

		asio::const_buffers_1 GetAsioBuffer() const
		{
			return asio::buffer((const void*) &data, sizeof(data));
		}
	};

	RawBuffer::RawBuffer(const char* data)
	{
		allocated_size = std::strlen(data);
		used_size = allocated_size;
		buffer = ::operator new(allocated_size);
		std::strcpy((char*)buffer, data);
	}

	RawBuffer::RawBuffer(size_t size)
		:allocated_size(size), used_size(0)
	{
		buffer = ::operator new(allocated_size);
	}

	asio::const_buffers_1 RawBuffer::GetAsioBuffer() const
	{
		return asio::buffer((const void*)buffer, used_size);
	}

//	asio::mutable_buffers_1 RawBuffer::GetAsioBuffer()
//	{
//		return asio::buffer(buffer, used_size);
//	}

	asio::mutable_buffers_1 RawBuffer::ResetAndGetAsioBuffer(size_t size)
	{
		BOOST_ASSERT(size <= allocated_size);
		used_size = size;
		return asio::buffer(buffer, used_size);
	}

	size_t RawBuffer::GetSize() const
	{
		return allocated_size;
	}

	size_t RawBuffer::GetUsedSize() const
	{
		return used_size;
	}

	string RawBuffer::CopyToString() const
	{
		return std::string((char*)buffer, used_size);
	}

	RawBuffer::~RawBuffer()
	{
		::operator delete(buffer);
	}

	RawBuffer::RawBuffer(const RawBuffer& src)
		:buffer(0),allocated_size(0),used_size(0) {}


	TCPTransceiver::TCPTransceiver(io_service& service, std::unique_ptr<ip::tcp::socket> socket, size_t receiveLimit)
		:service(service), socket(std::move(socket)), receiveLimit(receiveLimit) {}

	void TCPTransceiver::Send(const RawBuffer& buffer, boost::function<void()> done)
	{
		auto messageSizeToken = new SizeTToken(buffer.GetUsedSize());
		asio::async_write(*socket, messageSizeToken->GetAsioBuffer(), [this, done, messageSizeToken, &buffer](const system::error_code& error, size_t size) {
			delete messageSizeToken;

			if(error != 0)
				throw "Error sending message header";

			asio::async_write(*socket, buffer.GetAsioBuffer(), [done](const system::error_code& error, size_t size) {
				done();
			});
		});
	}

	void TCPTransceiver::Receive(std::function<void(std::unique_ptr<RawBuffer>)> done) // Would have liked to use the more efficient boost::function, but it is seems incompatible (any attempt at using the two together seems to result in the compiler complaining about use of deleted constructors...?) with std::unique_ptr and boost::unique_ptr is annoying to use because it has no default deleter.
	{
		auto messageSizeToken = new SizeTToken();
		asio::async_read(*socket, messageSizeToken->GetAsioBuffer(), [this, done, messageSizeToken](const system::error_code& error, size_t size) {
			auto wrapped_token = std::unique_ptr<SizeTToken>(messageSizeToken);
			if(error != 0)
				throw "Error receiving message header";

			size_t messageSize = wrapped_token->GetValue();
			if(messageSize > receiveLimit)
				throw "Error receiving message, message too large for buffer";

			auto buffer = new RawBuffer(messageSize);
			asio::async_read(*socket, buffer->ResetAndGetAsioBuffer(messageSize), [done, buffer](const system::error_code& error, size_t size) {
				auto wrapped_buffer = std::unique_ptr<RawBuffer>(buffer);
				if(error != 0)
					throw "Error receiving message header";

				done(std::move(wrapped_buffer));
			});
		});
	}

	//#define CHAT_BUFF_SIZE 16
	//class session  // Fix async exceptions!
	//{
	//public:
	//	session(uint32_t id, io_service& service, unique_ptr<ip::tcp::socket> socket)
	//		:id(id), service(service), socket(move(socket)) {}
	//
	//	void Setup(boost::function<void()> sessionReady)
	//	{
	//		SerializedSizeT requestMessageSize;
	//		boost::asio::async_read(*socket, boost::asio::buffer(requestMessageSize.GetDataPtr(), SerializedSizeT::DataSize()), [this, sessionReady, requestMessageSize](const boost::system::error_code& error, std::size_t size) {
	//			if(error != 0)
	//				throw "An error occured!";
	//
	//			if(size < SerializedSizeT::DataSize())
	//				throw "Received incomplete session request header";
	//
	//			size_t requestSize = requestMessageSize.GetValue();
	//			if(requestSize > chatBuffer.size())
	//				throw "Received too large session request";
	//
	//			boost::asio::async_read(*socket, boost::asio::buffer(chatBuffer, requestSize), [this, sessionReady, requestSize](const boost::system::error_code& error, std::size_t size) {
	//				if(error != 0)
	//					throw "Error reading session request";
	//
	//				if(size < requestSize) // Is this possible when there is no error?
	//					throw "Received incomplete Session request";
	//
	//				auto req = new SessionRequest();
	//				if(!req->ParseFromArray(chatBuffer.elems, size))
	//					throw "Received malformed Session request";
	//
	//				cout << "Got magick value from client " << req->magick() << endl;
	//
	//				SessionResponse response;
	//				response.set_sessionid(id);
	//				size_t responseSize = response.ByteSize();
	//				BOOST_ASSERT(responseSize < chatBuffer.size());
	//
	//				bool succ = response.SerializeToArray(chatBuffer.elems, chatBuffer.size());
	//				BOOST_ASSERT(succ);
	//
	//				boost::asio::async_write(*socket, boost::asio::buffer(chatBuffer, responseSize), [sessionReady, responseSize](const boost::system::error_code& error, size_t size) {
	//					if(error != 0)
	//						throw "Error writing session response!";
	//
	//					sessionReady();
	//				});
	//			});
	//		});
	//	}
	//
	//	uint32_t GetId() const
	//	{
	//		return id;
	//	}
	//
	//private:
	////	void hookup_async_read()
	////	{
	////		socket->async_read_some(boost::asio::buffer(*buffer, read_limit),
	////			boost::bind(&session::handle_async_read, this,
	////			boost::asio::placeholders::error,
	////			boost::asio::placeholders::bytes_transferred));
	////	}
	//
	////	void handle_async_read(const boost::system::error_code& error, std::size_t size)
	////	{
	////		if(error)
	////		{
	////			cerr << "Error receiving data " << error.message() << endl;
	////			completion(error);
	////			return;
	////		}
	////
	////
	////		hookup_async_read();
	////	}
	//
	//	uint32_t id;
	//	io_service& service;
	//	unique_ptr<ip::tcp::socket> socket;
	//	boost::array<char, CHAT_BUFF_SIZE> chatBuffer;
	//};

	Listener::Listener(boost::asio::io_service& service, short port)
		:service(service), acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)) {}

	void Listener::Listen(std::function<void(std::unique_ptr<TCPTransceiver>)> handler)
	{
		this->handler = handler;
		hookupAccept();
	}

	void Listener::hookupAccept()
	{
		auto socket = new ip::tcp::socket(service);
		acceptor.async_accept(*socket,
			boost::bind(&Listener::handleAccept, this, socket,
			boost::asio::placeholders::error));
	}

	void Listener::handleAccept(ip::tcp::socket* socket, const boost::system::error_code& error)
	{
		auto wrapped_socket = std::unique_ptr<ip::tcp::socket>(socket);
		if(error)
		{
			//TODO: Should the socket be freed here?
			cerr << "Error accepting connection " + error.message() << endl;
			hookupAccept();
			return;
		}

		handler(unique_ptr<TCPTransceiver>(new TCPTransceiver(service, std::move(wrapped_socket))));
				//new session(1, service, unique_ptr<ip::tcp::socket>(socket)));

		//hookup_async_accept();
	}

	Connector::Connector(boost::asio::io_service& service)
		:service(service) {}

	const string format_endpoint(const ip::tcp::endpoint& ep)
	{
		stringstream ss;
		const ip::address& ip = ep.address();
		if(ip.is_v4())
		{
			ss << ip.to_string() << ':' << ep.port();
			return ss.str();
		}
		else if (ip.is_v6())
		{
			ss << "[" << ip.to_string() << "]:" << ep.port();
			return ss.str();
		}

		throw "Unknown address type!?";
	}

	void Connector::Connect(const string& server, const string& port, std::function<void(std::unique_ptr<TCPTransceiver>)> handler)
	{
		auto resolver = new ip::tcp::resolver(service);
		resolver->async_resolve(ip::tcp::resolver::query(server, port), [this, handler, resolver](const boost::system::error_code& err, ip::tcp::resolver::iterator endpoint_iterator) { // for some reason the query does not need to be kept alive during the async processing
			auto uResolver = std::unique_ptr<ip::tcp::resolver>(resolver);
			if(err)
				throw "Error resolving hostname: " + err.message();

			cout << "Server resolved to ";
			auto end = ip::tcp::resolver::iterator();
			for(ip::tcp::resolver::iterator epi = endpoint_iterator; epi != end; ++epi)
				cout << format_endpoint(*epi) << ", ";
			cout << endl;

			auto socket = new ip::tcp::socket(service);
			async_connect(*socket, endpoint_iterator, [this, handler, socket](const boost::system::error_code& err, ip::tcp::resolver::iterator endpoint_iterator) {
				auto uSocket = std::unique_ptr<ip::tcp::socket>(socket);
				if(err)
					throw "Error connecting " + err.message();

				cout << "Connected to " << format_endpoint(*endpoint_iterator) << endl;

				handler(std::unique_ptr<TCPTransceiver>(new TCPTransceiver(service, std::move(uSocket))));
			});
		});

		//auto socket = new ip::tcp::socket(service);
	}
}
