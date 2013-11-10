#include <iostream>
#include <sstream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio/buffer.hpp>

using namespace std;
using namespace boost;
using namespace boost::asio;

class session
{
public:
	typedef void (*completion_handler)(const boost::system::error_code& error);

	session(io_service& service, unique_ptr<ip::tcp::socket> socket)
		:service(service), socket(move(socket)) {}

	void read(boost::array<char, 1024>* buffer, int length, const completion_handler& done)
	{
		boost::asio::async_read(*socket, boost::asio::buffer(*buffer, length), [&](const boost::system::error_code& error, std::size_t size) { done(error); });
	}

private:
//	void hookup_async_read()
//	{
//		socket->async_read_some(boost::asio::buffer(*buffer, read_limit),
//			boost::bind(&session::handle_async_read, this,
//			boost::asio::placeholders::error,
//			boost::asio::placeholders::bytes_transferred));
//	}

//	void handle_async_read(const boost::system::error_code& error, std::size_t size)
//	{
//		if(error)
//		{
//			cerr << "Error receiving data " << error.message() << endl;
//			completion(error);
//			return;
//		}
//
//
//		hookup_async_read();
//	}

	io_service& service;
	unique_ptr<ip::tcp::socket> socket;
};

class listener
{
public:
	typedef void (*new_session_handler)(session* new_session);

	listener(boost::asio::io_service& service, short port)
    	:service(service), acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)) {}

	void listen(const new_session_handler* handler)
	{
		this->handler = handler;
		hookup_async_accept();
	}

private:
	void hookup_async_accept()
	{
		auto socket = new ip::tcp::socket(service);
		acceptor.async_accept(*socket,
			boost::bind(&listener::handle_accept, this, socket,
			boost::asio::placeholders::error));
	}

	void handle_accept(ip::tcp::socket* socket, const boost::system::error_code& error)
	{
		if(error)
		{
	    	cerr << "Error accepting connection " + error.message() << endl;
			hookup_async_accept();
			return;
		}

		(*handler)(new session(service, unique_ptr<ip::tcp::socket>(socket)));

		hookup_async_accept();
	}

private:
	io_service& service;
	ip::tcp::acceptor acceptor;
	const new_session_handler* handler;
};



//class tcp_transceiver
//{
//	tcp_transceiver(io_service& service)
//		:service(service)
//	{
//		socket = ip::tcp::socket();
//	}
//
//private:
//	boost::asio::io_service& service;
//	ip::udp::socket *socket;
//
//	ip::udp::endpoint remote_endpoint_;
//	boost::array<char, 1024> recv_buffer;
//};


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

int main()
{
	stringstream ss;

 	ss << "Hahahahhahah";

 	string s = ss.str();

 	cout << s << endl;


	//udp::resolver resolver(io_service);
    //udp::resolver::query query(udp::v4(), peer, "8080");
    //this->peer = *resolver.resolve(query);

	io_service service;
	udp_transceiver transceiver(service);

	cout << "Server running:" << endl;

	transceiver.run();

	cout << "Server done...?" << endl;

	char dummy;
	cin >> dummy;

	return 0;
}

