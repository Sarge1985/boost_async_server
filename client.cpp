#ifdef WIN32
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#endif


#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
//#include <boost/multiprecision/mpfr.hpp>
//#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/exception/enable_current_exception.hpp>
#include <memory>
#include <iostream>
using namespace boost::asio;
io_service service;

#define FN_1(x)       boost::bind(&self_type::x, shared_from_this())
#define FN_2(x,y)    boost::bind(&self_type::x, shared_from_this(),y)
#define FN_3(x,y,z)  boost::bind(&self_type::x, shared_from_this(),y,z)
#define MAX_VAL 18446744073709551615

typedef struct 
{
	unsigned long long int a;
	unsigned long long int b;
	unsigned long long int result;
}my_data;


ip::tcp::endpoint localhost(ip::address::from_string("127.0.0.1"), 8001);

class async_client : public boost::enable_shared_from_this<async_client>
	, boost::noncopyable 
{
private:
	ip::tcp::socket sock_;
	char* read_buffer_;
	char* write_buffer_;
	char* message_;
	bool  started_;
	typedef async_client self_type;
	async_client(char* buf): sock_(service), started_(true), message_(buf) {}

	void start_(ip::tcp::endpoint ep) 
	{
		sock_.async_connect(ep, FN_2(on_connect, _1));
	}

public:
	typedef boost::system::error_code error_code;
	typedef boost::shared_ptr<async_client> ptr;
	
	static bool is_uint(const std::string& s)
	{
		return s.find_first_not_of("0123456789") == std::string::npos;
	}
	static ptr start(ip::tcp::endpoint ep)
	{
		my_data *values = new my_data;
		std::string buf_a;
		std::string buf_b;
		while (1) 
		{
			std::cout << "Please, input val_1" << std::endl;
			std::getline(std::cin, buf_a);
			if (!is_uint(buf_a) || strlen(buf_a.data()) == 0)
				continue;
			try
			{			
				values->a = boost::lexical_cast<unsigned long long int> (buf_a);
			}
			catch (std::exception const&  ex) {
				std::cout << "The result will be overcrowded, please input new values" << std::endl;
				continue;
			};
			
			std::cout << "Please, input val_2" << std::endl;
			std::getline(std::cin, buf_b);
			if (!is_uint(buf_b) || strlen(buf_b.data()) == 0)
				continue;
			try 
			{
				values->b = boost::lexical_cast<unsigned long long int> (buf_b);
			}
			catch (std::exception const&  ex) 
			{
				std::cout << "The result will be overcrowded, please input new values" << std::endl;
				continue;
			};
			unsigned long long int tmp = values->a*values->b;
			if (values->b == 0)
				std::swap(values->a, values->b);
			if (values->b == 0 && values->a == 0)
			{
				std::cout << "You input value a = 0 and value b = 0, please input new values" << std::endl;
				continue;
			}
			if ((tmp/values->b) != values->a) 
			{
				std::cout << "The MULTIPLE result will be overcrowded, please input new values" << std::endl;
				continue;
			}

			if (is_uint(buf_a) && is_uint(buf_b))
				break;
		}
		ptr new_(new async_client(reinterpret_cast<char*>(values)));
		new_->start_(ep);
		return new_;
	}
	void stop() 
	{
		if (!started_) 
			return;
		started_ = false;
		sock_.close();
	}
	bool started() 
	{ 
		return started_; 
	}

private:
	void on_connect(const error_code & err) 
	{
		if (!err) 
		{
			do_write(message_);
			delete message_;
		}	
		else 
		{
			std::cout << "Server not started, end working..." << std::endl;
			boost::this_thread::sleep(boost::posix_time::millisec(2000));
			stop();
		}
	}
	void on_read(const error_code & err, size_t bytes) 
	{
		if (!err) 
		{
			my_data* data = reinterpret_cast<my_data*>(read_buffer_);
			std::cout << data->a << " " << data->b<< " Result = " << data->result << std::endl;
		}
		start(localhost);
	}
	void on_write(const error_code & err, size_t bytes) 
	{
		do_read();
	}
	void do_read() 
	{
		read_buffer_ = new char[sizeof(my_data)];
		std::shared_ptr<char> p(read_buffer_, std::default_delete<char[]>());
		async_read(sock_, buffer(p.get(), sizeof(my_data)),
			FN_3(read_complete, _1, _2), FN_3(on_read, _1, _2));
	}
	void do_write(char *data) 
	{
		if (!started()) 
			return;
		write_buffer_ = data;
		sock_.async_write_some(buffer(write_buffer_, sizeof(my_data)),
			FN_3(on_write, _1, _2));
	}
	size_t read_complete(const boost::system::error_code & err, size_t bytes) 
	{
		if (err) 
			return 0;		
		else
			return 1;
	}
};

int main(int argc, char* argv[]) 
{
	async_client::start(localhost);
	boost::this_thread::sleep(boost::posix_time::millisec(1000));
	service.run();
}
