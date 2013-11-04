#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/asio/buffer.hpp>

using namespace std;
using namespace boost::asio;

void handle_receive(const boost::system::error_code& error, std::size_t size)
{
	cout << "incoming data.." << endl;
}

int main()
{
	io_service io_service;
	ip::udp::socket *socket = new ip::udp::socket(io_service, ip::udp::endpoint(ip::udp::v4(), 5678));

	ip::udp::endpoint remote_endpoint_;
	boost::array<char, 1024> recv_buffer;

	socket->async_receive_from(boost::asio::buffer(recv_buffer), remote_endpoint_,
			[&recv_buffer] (const boost::system::error_code& error, std::size_t size)
			{
				recv_buffer[size >= 1024 ? 1023 : size] = 0;
				cout << "incoming data: " << recv_buffer.elems << endl;
			});
	//udp::resolver resolver(io_service);
    //udp::resolver::query query(udp::v4(), peer, "8080");
    //this->peer = *resolver.resolve(query);

	cout << "Server running:" << endl;
	io_service.run();

	cout << "Server done...?" << endl;

	char dummy;
	cin >> dummy;

	return 0;
}

