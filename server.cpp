#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#endif

#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/multiprecision/cpp_int.hpp>
using namespace boost::asio;
using namespace boost::posix_time;

io_service service;

#define FN_1(x)       boost::bind(&self_type::x, shared_from_this())
#define FN_2(x,y)    boost::bind(&self_type::x, shared_from_this(),y)
#define FN_3(x,y,z)  boost::bind(&self_type::x, shared_from_this(),y,z)

typedef struct {
	unsigned long long int a;
	unsigned long long int b;
	unsigned long long int result;
}my_data;

my_data *values;

class async_server : public boost::enable_shared_from_this<async_server>, boost::noncopyable {

private:
	ip::tcp::socket sock_;
	char *read_buffer_;
	char *write_buffer_;
	bool started_;
	typedef async_server self_type;
	async_server() : sock_(service), started_(false) {}

public:
	typedef boost::system::error_code error_code;
	typedef boost::shared_ptr<async_server> ptr;

	static unsigned long long int  gcd(unsigned long long int a, unsigned long long int b)
	{
		if (a < b)
		{
			std::swap(a, b);
		}
		while (b)
		{
			a %= b;
			std::swap(a, b);
		}
		return a;
	}
	static  unsigned long long int  least_common_mult(my_data* d)
	{
		if (d->a < d->b)
		{
			std::swap(d->a, d->b);
		}
		if ((d->a - d->b) == 1)
			return d->a*d->b;
		if ((d->a - d->b) == 0)
			return d->a;
		return d->a / gcd(d->a, d->b) * d->b;
	}
	void start() 
	{
		started_ = true;
		do_read();
	}
	static ptr new_() 
	{
		ptr new_(new async_server);
		return new_;
	}
	void stop() 
	{
		if (!started_) return;
		started_ = false;
		sock_.close();
	}
	ip::tcp::socket & sock() { return sock_; }

	void on_read(const error_code & err, size_t bytes) 
	{
		if (!err) 
		{
			my_data *d =  (my_data*)read_buffer_;
			std::cout << "Calculate... "<< d->a << " " << d->b<< std::endl;	
			d->result=least_common_mult(d);
			std::cout << "Send result "<< d->result<< std::endl;
			do_write((char*)d);
		}
		stop();
	}
	void on_write(const error_code & err, size_t bytes) 
	{
		do_read();
	}
	void do_read() 
	{
		read_buffer_ = new char[sizeof(my_data)];
		async_read(sock_, buffer(read_buffer_,sizeof(my_data)),
			FN_3(read_complete, _1, _2), FN_3(on_read, _1, _2));
	}
	void do_write(char* msg) 
	{
		sock_.async_write_some(buffer(msg, sizeof(my_data)),
			FN_3(on_write, _1, _2));
		delete read_buffer_;
	}
	size_t read_complete(const boost::system::error_code & err, size_t bytes) 
	{
		if (err) return 0;
		else
			return 1;
	}
};

ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));

void handle_accept(async_server::ptr client, const boost::system::error_code & err) 
{
	client->start();
	async_server::ptr new_client = async_server::new_();
	acceptor.async_accept(new_client->sock(), boost::bind(handle_accept, new_client, _1));
}


int main(int argc, char* argv[]) 
{
	async_server::ptr client = async_server::new_();
	acceptor.async_accept(client->sock(), boost::bind(handle_accept, client, _1));
	service.run();
}
