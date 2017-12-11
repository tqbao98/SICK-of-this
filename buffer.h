#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdlib.h>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/asio.hpp>
#include <boost/lambda/lambda.hpp>
#include <algorithm>
#include <iterator>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <boost/bind.hpp>

using namespace std;

class SickTimCommonTcp 
{
public:
	//reference data types changed
	//protected->public!!!
	SickTimCommonTcp(int timelimit);
	virtual ~SickTimCommonTcp();

	virtual int init_device();
	virtual int close_device();
	/// Send a SOPAS command to the device and print out the response to the console.
	virtual int sendSOPASCommand(const char* request/*, std::vector<unsigned char> * reply*/);

	virtual int get_datagram(char* receiveBuffer, int bufferSize, int* actual_length);

	// Helpers for boost asio
	int readWithTimeout(size_t timeout_ms, char *buffer, int buffer_size, int *bytes_read = 0, bool *exception_occured = 0);
	void handleRead(boost::system::error_code error, size_t bytes_transfered);
	void checkDeadline();

private:
	boost::asio::io_service io_service_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::deadline_timer deadline_;
	boost::asio::streambuf input_buffer_;
	boost::system::error_code ec_;
	size_t bytes_transfered_;

	std::string hostname_;
	std::string port_;
	int timelimit_;
};

inline void SickTimCommonTcp::handleRead(boost::system::error_code error, size_t bytes_transfered)
{
	ec_ = error;
	bytes_transfered_ += bytes_transfered;
}


#endif
