#ifndef NETUTILS_H_
#define NETUTILS_H_
#include <boost/asio/buffer.hpp>
#include <boost/asio.hpp>
#include <memory>
//#include <string>
#include <boost/function.hpp>

namespace NetUtils
{
	using namespace std;
	using namespace boost;
	using namespace boost::asio;

	class RawBuffer
	{
	public:
		explicit RawBuffer(size_t size);
		asio::const_buffers_1 GetAsioBuffer() const;
		asio::mutable_buffers_1 ResetAndGetAsioBuffer(size_t size);
		size_t GetSize() const;
		size_t GetUsedSize() const;
		string CopyToString() const;
		~RawBuffer();
	private:
		void *buffer;
		size_t allocated_size;
		size_t used_size;

		RawBuffer(const RawBuffer& src);
	};

	class TCPTransceiver
	{
	public:
		TCPTransceiver(io_service& service, std::unique_ptr<ip::tcp::socket> socket, size_t receiveLimit = 65536);
		void Send(const RawBuffer& buffer, boost::function<void()> done);
		void Receive(std::function<void(std::unique_ptr<RawBuffer>)> done);

	private:
		io_service& service;
		std::unique_ptr<ip::tcp::socket> socket;
		size_t receiveLimit;
	};

	class Listener
	{
	public:
		Listener(boost::asio::io_service& service, short port);
		void Listen(std::function<void(std::unique_ptr<TCPTransceiver>)> handler);
	private:
		void hookupAccept();
		void handleAccept(ip::tcp::socket* socket, const boost::system::error_code& error);

		io_service& service;
		ip::tcp::acceptor acceptor;
		std::function<void(std::unique_ptr<TCPTransceiver>)> handler;
		//boost::function<void(session* new_session)> handler;
	};
}
#endif
