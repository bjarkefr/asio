//#include <exception> // for std::bad_alloc
//#include <new>
//#include <cstdlib> // for malloc() and free()

#include <iostream>
#include <sstream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/function.hpp>

using namespace std;
using namespace boost;
using namespace boost::asio;

class session
{
public:
	typedef void (completion_handler)(const boost::system::error_code& error, size_t size);

	session(io_service& service, unique_ptr<ip::tcp::socket> socket)
		:service(service), socket(move(socket)) {}

	void read(boost::array<char, 1024>* buffer, size_t length, completion_handler done)
	{
		boost::asio::async_read(*socket, boost::asio::buffer(*buffer, length), [&](const boost::system::error_code& error, std::size_t size) {
			done(error, size);
		});
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
	typedef void (new_session_handler)(session* new_session);

	listener(boost::asio::io_service& service, short port)
    	:service(service), acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)) {}

	void set_new_session_handler(boost::function<void(session* new_session)>& handler)
	{
		this->handler = &handler;
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
	boost::function<void(session* new_session)>* handler;
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

	io_service service;
	listener listen(service, 555);

	//boost::function<void(session*)> session_h = [](session* nsession) {};

	//listen.set_new_session_handler(&session_h);
	listen.set_new_session_handler(boost::function<void(session*)>([&](session *nsession) { }));

				//[](session* nsession) {
//		cout << "New session accepted!" << endl;
//		boost::array<char, 1024> chars;
//		nsession->read(&chars, chars.size() - 1, [&chars](const boost::system::error_code& error, size_t size) {
//			chars.back() = 0;
//			cout << "Received data (length" << size << ")" << endl;
//			cout << chars.elems << endl;
//		});
	//});
//
//	service.run();
//
//	stringstream ss;
//
// 	ss << "Hahahahhahah";
//
// 	string s = ss.str();
//
// 	cout << s << endl;
//
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

