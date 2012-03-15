#include <iostream>
#include <iomanip>
#include "IoHandler.h"
#include "Options.h"
#include "WmrMessage.h"

IoHandler::IoHandler(const std::string& host, const std::string& port, Database& db) :
    boost::asio::io_service(),
    m_socket(*this),
    m_db(db),
    m_active(true)
{
    boost::asio::ip::tcp::resolver resolver(*this);
    boost::asio::ip::tcp::resolver::query query(host, port);
    boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(query);

    m_socket.async_connect(*endpoint,
			   boost::bind(&IoHandler::handleConnect, this,
				       boost::asio::placeholders::error));

    /* pre-alloc buffer to avoid reallocations */
    m_data.reserve(64);
}

IoHandler::~IoHandler()
{
    if (m_active) {
	m_socket.close();
    }
}

void
IoHandler::handleConnect(const boost::system::error_code& error)
{
    if (error) {
	doClose(error);
    } else {
	readStart();
    }
}

void
IoHandler::readComplete(const boost::system::error_code& error,
			size_t bytesTransferred)
{
    size_t pos = 0, count;
    DebugStream& debug = Options::ioDebug();

    if (error) {
	doClose(error);
	return;
    }

    if (debug) {
	debug << "IO: Got bytes ";
	for (size_t i = 0; i < bytesTransferred; i++) {
	    debug << std::setfill('0') << std::setw(2)
		  << std::showbase << std::hex
		  << (unsigned int) m_recvBuffer[i] << " ";
	}
	debug << std::endl;
    }

    while (pos < bytesTransferred) {
	m_data.push_back(m_recvBuffer[pos++]);
	count = m_data.size();

	if (count > 1 && m_data[count - 1] == 0xff && m_data[count - 2] == 0xff) {
	    std::vector<uint8_t>::iterator end = m_data.begin() + count - 2;
	    std::vector<uint8_t> packet(m_data.begin(), end);
	    if (packet.size() < 2) {
		continue;
	    }
	    /* throw away junk at beginning */
	    for (size_t i = 0; i < packet.size() - 1; i++) {
		if (packet[i] == 0xff && packet[i + 1] == 0xff) {
		    packet.erase(packet.begin(), packet.begin() + i + 2);
		    break;
		}
	    }

	    /* handle packet */
	    WmrMessage message(packet, m_db);
	    if (message.isValid()) {
		message.parse();
	    }

	    m_data.erase(m_data.begin(), end);
	}
    }

    readStart();
}

void
IoHandler::doClose(const boost::system::error_code& error)
{
    if (error == boost::asio::error::operation_aborted) {
	/* if this call is the result of a timer cancel() */
	return; // ignore it because the connection cancelled the timer
    }

    if (error) {
	std::cerr << "Error: " << error.message() << std::endl;
    }

    m_socket.close();
    m_active = false;
}
