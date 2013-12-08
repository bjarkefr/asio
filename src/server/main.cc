//#define BOOST_DISABLE_ASSERTS
#include "../NetUtils.h"

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
//#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
//#include <boost/move/algorithm.hpp> // TODO: try:: boost::move!
#include <boost/function.hpp>
#include <boost/assert.hpp>
#include "../contract/SessionRequest.pb.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace NetUtils;

//uint32_t htonl(uint32_t hostlong);
//uint16_t htons(uint16_t hostshort);
//uint32_t ntohl(uint32_t netlong);
//uint16_t ntohs(uint16_t netshort);


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
	try {

	int h = 3;

	boost::function<int(int)> add_h = [&](int x) { return x + h; };

	//function<int(int)> kryl = add_h;

	cout << sizeof(add_h) << endl;

	//cout << call_b(add_h, 2) << endl;


	io_service service;
	//TCPTransceiver transceiver();
	//io_service& service, std::unique_ptr<ip::tcp::socket> socket, size_t receiveLimit = 65536

	Listener listen(service, 555);

	listen.Listen([](std::unique_ptr<TCPTransceiver> transceiver) {
		cout << "New session accepted!" << endl;

		auto transceiverP = transceiver.release();
		transceiverP->Receive([transceiverP](std::unique_ptr<RawBuffer> buffer) {
			cout << "Got data: " << buffer->CopyToString() << endl;
			delete transceiverP;
		});
	});

//	listen.Listen([](session* nsession) {
//		cout << "New session accepted!" << endl;
//		nsession->Setup([nsession]() {
//			cout << "Session set up. (id: " << nsession->GetId() << ")" << endl;
//		});
//	});
//
	service.run();

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
	}
	catch(const char* s)
	{
		cout << s << endl;
	}
	return 0;
}

