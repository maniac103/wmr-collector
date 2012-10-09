#ifndef __IOHANDLER_H__
#define __IOHANDLER_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <fstream>
#include "Database.h"

class IoHandler : public boost::asio::io_service
{
    public:
	IoHandler(const std::string& host, const std::string& port, boost::shared_ptr<Database>& db);
	~IoHandler();

	void close() {
	    post(boost::bind(&IoHandler::doClose, this,
			     boost::system::error_code()));
	}

	bool active() {
	    return m_active;
	}

    private:
	/* maximum amount of data to read in one operation */
	static const int maxReadLength = 512;

	void readStart() {
	    /* Start an asynchronous read and call read_complete when it completes or fails */
	    m_socket.async_read_some(boost::asio::buffer(m_recvBuffer, maxReadLength),
				     boost::bind(&IoHandler::readComplete, this,
				     boost::asio::placeholders::error,
				     boost::asio::placeholders::bytes_transferred));
	}

	void handleConnect(const boost::system::error_code& error);
	void readComplete(const boost::system::error_code& error, size_t bytesTransferred);
	void doClose(const boost::system::error_code& error);
	void resetWatchdog();
	void watchdogTimeout(const boost::system::error_code& error);

    private:
	enum {
	    StartMarker,
	    Flags,
	    Type,
	    Data
	} m_state;

	size_t m_pos;
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::deadline_timer m_watchdog;
	boost::shared_ptr<Database> m_db;
	bool m_active;
	unsigned char m_recvBuffer[maxReadLength];
	std::vector<uint8_t> m_data;
};

#endif /* __IOHANDLER_H__ */
