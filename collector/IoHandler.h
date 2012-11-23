/*
 * Oregon WMR88/WMR88A data collection daemon
 *
 * Copyright (C) 2012 Danny Baumann <dannybaumann@web.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
