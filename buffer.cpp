#include "buffer.h"
#include <iostream>

//reference data types changed
//use fixed values for hostname, port and timelimit -> eliminate parameters??
SickTimCommonTcp::SickTimCommonTcp(int timelimit) 
	:
	socket_(io_service_),
	deadline_(io_service_),
	timelimit_(timelimit)
{
	deadline_.expires_at(boost::posix_time::pos_infin);
	checkDeadline();
}

SickTimCommonTcp::~SickTimCommonTcp()
{
	close_device();
}

using boost::asio::ip::tcp;
using boost::lambda::var;
using boost::lambda::_1;

int SickTimCommonTcp::init_device()
{
	// Resolve the supplied hostname
	tcp::resolver::iterator iterator;
	try
	{
		tcp::resolver resolver(io_service_);
		tcp::resolver::query query("169.254.138.124", "2112");
		iterator = resolver.resolve(query);
	}
	catch (boost::system::system_error &e)
	{
		cout << "error in init TCP" << &e << endl;
		return -1;
	}

	// Try to connect to all possible endpoints
	boost::system::error_code ec;
	bool success = false;
	for (; iterator != tcp::resolver::iterator(); ++iterator)
	{
		std::string repr = boost::lexical_cast<std::string>(iterator->endpoint());
		socket_.close();

		//// Set the time out length
		//cout << "waiting time out " << timelimit_ << endl;
		//deadline_.expires_from_now(boost::posix_time::seconds(/*timelimit_*/5000));
		//ec = boost::asio::error::would_block;
		
		cout << "connecting to " << repr.c_str() << endl;
		boost::asio::connect(socket_, iterator); //asynchronous

		//// Wait until timeout
		//do io_service_.run_one(); while (ec == boost::asio::error::would_block);

		if (!ec && socket_.is_open())
		{
			success = true;
			cout << "connected" << endl;
			break;
		}
		cout << "failed to connect to " << repr.c_str() << endl;
	}

	// Check if connecting succeeded
	if (!success)
	{
		cout << "cannot connect to " << hostname_.c_str() << port_.c_str() << endl;
		return -1;
	}

	input_buffer_.consume(input_buffer_.size());

	return 0;
}

int SickTimCommonTcp::close_device()
{
	if (socket_.is_open())
	{
		try
		{
			socket_.close();
		}
		catch (boost::system::system_error &e)
		{
			cout << "can't close " << e.code().value() << e.code().message().c_str() << endl;
		}
	}
	return 0;
}

void SickTimCommonTcp::checkDeadline()
{
	if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		// The reason the function is called is that the deadline expired. Close
		// the socket to return all IO operations and reset the deadline
		socket_.close();
		deadline_.expires_at(boost::posix_time::pos_infin);
	}

	// Nothing bad happened, go back to sleep
	deadline_.async_wait(boost::bind(&SickTimCommonTcp::checkDeadline, this));
}

int SickTimCommonTcp::readWithTimeout(size_t timeout_ms, char *buffer, int buffer_size, int *bytes_read, bool *exception_occured)
{
	// Set up the deadline to the proper timeout, error and delimiters
	deadline_.expires_from_now(boost::posix_time::milliseconds(timeout_ms));
	const char end_delim = static_cast<char>(0x03);
	ec_ = boost::asio::error::would_block;
	bytes_transfered_ = 0;

	// Read until 0x03 ending indicator
	boost::asio::async_read_until(
		socket_,
		input_buffer_,
		end_delim,
		boost::bind(
			&SickTimCommonTcp::handleRead,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
	do io_service_.run_one(); while (ec_ == boost::asio::error::would_block);

	if (ec_)
	{
		// would_block means the connectio is ok, but nothing came in in time.
		// If any other error code is set, this means something bad happened.
		if (ec_ != boost::asio::error::would_block)
		{
			cout << "sendSOPASCommand: failed attempt to read from socket: %d: %s" << ec_.value() << ec_.message().c_str() <<endl;
			if (exception_occured != 0)
				*exception_occured = true;
		}

		// For would_block, just return and indicate nothing bad happend
		return -1;
	}

	// Avoid a buffer overflow by limiting the data we read
	size_t to_read = bytes_transfered_ > buffer_size - 1 ? buffer_size - 1 : bytes_transfered_;
	std::istream istr(&input_buffer_);
	if (buffer != 0)
	{
		istr.read(buffer, to_read);
		buffer[to_read] = 0;

		// Consume the rest of the message if necessary
		if (to_read < bytes_transfered_)
		{
			cout << "Dropping "<< bytes_transfered_ - to_read << " bytes to avoid buffer overflow"<< endl;
			input_buffer_.consume(bytes_transfered_ - to_read);
		}
	}
	else
		// No buffer was provided, just drop the data
		input_buffer_.consume(bytes_transfered_);

	// Set the return variable to the size of the read message
	if (bytes_read != 0)
		*bytes_read = to_read;

	return -1;
}

/**
* Send a SOPAS command to the device and print out the response to the console.
*/
int SickTimCommonTcp::sendSOPASCommand(const char* request/*, std::vector<unsigned char> * reply*/)
{
	if (!socket_.is_open()) {
		cout << "sendSOPASCommand: socket not open" << endl;
		return -1;
	}

	/*
	* Write a SOPAS variable read request to the device.
	*/
	try
	{
		boost::asio::write(socket_, boost::asio::buffer(request, strlen(request)));
	}
	catch (boost::system::system_error &e)
	{
		cout << "write error for command: %s" << request << endl;
		return -1;
	}
	// Set timeout in 5 seconds
	const int BUF_SIZE = 1000;
	char buffer[BUF_SIZE];
	int bytes_read;
	if (readWithTimeout(1000, buffer, BUF_SIZE, &bytes_read, 0) == -1)
	{
		cout << "sendSOPASCommand: no full reply available for read after 1s" << endl;
		return -1;
	}

	//if (reply)
	//{
	//	reply->resize(bytes_read);
	//	std::copy(buffer, buffer + bytes_read, &(*reply)[0]);
	//}

	return 0;
}

int SickTimCommonTcp::get_datagram(char* receiveBuffer, int bufferSize, int* actual_length)
{
	if (!socket_.is_open()) {
		cout << "datagram: socket not open" << endl;
		return -1;
	}

	/*
	* Write a SOPAS variable read request to the device.
	*/
	std::vector<unsigned char> reply;

	// Wait at most 1000ms for a new scan
	size_t timeout = 1000;
	bool exception_occured = false;

	char *buffer = reinterpret_cast<char *>(receiveBuffer);

	if (readWithTimeout(timeout, buffer, bufferSize, actual_length, &exception_occured) != 0)
	{
		cout << "get_datagram: no data available for read after "<< timeout << " ms" << endl;
			// Attempt to reconnect when the connection was terminated
		if (!socket_.is_open())
		{
			cout << "reconnecting" << endl;
		}

		return exception_occured ? -1 : 0;  // keep on trying
	}

	return 0;
}
