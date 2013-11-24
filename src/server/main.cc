//#define BOOST_DISABLE_ASSERTS

//#include <exception> // for std::bad_alloc
//#include <new>
//#include <cstdlib> // for malloc() and free()

#include <iostream>
#include <sstream>
#include <memory>
#include <algorithm>
#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/function.hpp>
#include <boost/assert.hpp>
#include "../contract/SessionRequest.pb.h"

//uint32_t htonl(uint32_t hostlong);
//uint16_t htons(uint16_t hostshort);
//uint32_t ntohl(uint32_t netlong);
//uint16_t ntohs(uint16_t netshort);

using namespace std;
using namespace boost;
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

	void run()
	{
		service.run();
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

class RawBuffer
{
	void *buffer;
	size_t allocated_size;
	size_t used_size;

	RawBuffer(const RawBuffer& src)
		:buffer(0),allocated_size(0),used_size(0) {}

public:
	explicit RawBuffer(size_t size)
		:allocated_size(size), used_size(0)
	{
		buffer = ::operator new(allocated_size);
	}

	asio::const_buffers_1 GetAsioBuffer() const
	{
		return asio::buffer((const void*)buffer, used_size);
	}

//	asio::mutable_buffer GetAsioBuffer()
//	{
//		return asio::buffer(buffer, used_size);
//	}

	asio::mutable_buffers_1 ResetAndGetAsioBuffer(size_t size)
	{
		BOOST_ASSERT(size < allocated_size);
		used_size = size;
		return asio::buffer(buffer, used_size);
	}

	size_t GetSize() const
	{
		return allocated_size;
	}

	size_t GetUsedSize() const
	{
		return used_size;
	}

	~RawBuffer()
	{
		::operator delete(buffer);
	}
};

class TCPTransceiver
{
	io_service& service;
	unique_ptr<ip::tcp::socket> socket;

public:
	TCPTransceiver(io_service& service, unique_ptr<ip::tcp::socket> socket)
		:service(service), socket(move(socket)) {}

	void Send(RawBuffer& buffer, boost::function<void()> done)
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

	void Receive(RawBuffer& buffer, boost::function<void()> done)
	{
		auto messageSizeToken = new SizeTToken();
		asio::async_read(*socket, messageSizeToken->GetAsioBuffer(), [this, done, messageSizeToken, &buffer](const system::error_code& error, size_t size) {
			auto wrapped_token = unique_ptr<SizeTToken>(messageSizeToken);
			if(error != 0)
				throw "Error receiving message header";

			size_t messageSize = wrapped_token->GetValue();
			if(messageSize > buffer.GetSize())
				throw "Error receiving message, message too large for buffer";

			asio::async_read(*socket, buffer.ResetAndGetAsioBuffer(messageSize), [done](const system::error_code& error, size_t size) {
				if(error != 0)
					throw "Error receiving message header";

				done();
			});
		});
	}
};

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
//
//class listener
//{
//public:
//	//typedef void (new_session_handler)(session* new_session);
//
//	listener(boost::asio::io_service& service, short port)
//    	:service(service), acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)) {}
//
//	void set_new_session_handler(boost::function<void(session* new_session)> handler)
//	{
//		this->handler = handler;
//		hookup_async_accept();
//	}
//
//private:
//	void hookup_async_accept()
//	{
//		auto socket = new ip::tcp::socket(service);
//		acceptor.async_accept(*socket,
//			boost::bind(&listener::handle_accept, this, socket,
//			boost::asio::placeholders::error));
//	}
//
//	void handle_accept(ip::tcp::socket* socket, const boost::system::error_code& error)
//	{
//		if(error)
//		{
//	    	cerr << "Error accepting connection " + error.message() << endl;
//			hookup_async_accept();
//			return;
//		}
//
//		handler(new session(1, service, unique_ptr<ip::tcp::socket>(socket)));
//
//		//hookup_async_accept();
//	}
//
//private:
//	io_service& service;
//	ip::tcp::acceptor acceptor;
//	boost::function<void(session* new_session)> handler;
//};

//int call_b(boost::function<int(int)> fnc, int x)
//{
//	return fnc(x);
//}

//void* operator new (size_t size)
//{
//	cout << "Alloc request " << size << endl;
//	void *p=malloc(size);
//	if (p==0) // did malloc succeed?
//		throw std::bad_alloc(); // ANSI/ISO compliant behavior
//	return p;
//}
//
//void operator delete (void *p) noexcept (true)
//{
//	free(p);
//}

int main()
{
	int h = 3;

	boost::function<int(int)> add_h = [&](int x) { return x + h; };

	//function<int(int)> kryl = add_h;

	cout << sizeof(add_h) << endl;

	//cout << call_b(add_h, 2) << endl;


//	io_service service;
//	listener listen(service, 555);
//
//	listen.set_new_session_handler([](session* nsession) {
//		cout << "New session accepted!" << endl;
//		nsession->Setup([nsession]() {
//			cout << "Session set up. (id: " << nsession->GetId() << ")" << endl;
//		});
//	});
//
//	service.run();

//	udp_transceiver transceiver(service);
//
//	cout << "Server running:" << endl;
//
//	transceiver.run();
//
//	cout << "Server done...?" << endl;
//
//	char dummy;
//	cin >> dummy;

	return 0;
}

