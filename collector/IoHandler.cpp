#include <iostream>
#include <iomanip>
#include "IoHandler.h"
#include "Options.h"
#include "WmrMessage.h"

IoHandler::IoHandler(const std::string& host, const std::string& port, boost::shared_ptr<Database>& db) :
    boost::asio::io_service(),
    m_state(StartMarker),
    m_pos(0),
    m_socket(*this),
    m_watchdog(*this),
    m_db(db),
    m_active(true)
{
    boost::system::error_code error;
    boost::asio::ip::tcp::resolver resolver(*this);
    boost::asio::ip::tcp::resolver::query query(host, port);
    boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(query, error);

    if (error) {
	doClose(error);
    } else {
	m_socket.async_connect(*endpoint,
			       boost::bind(&IoHandler::handleConnect, this,
					   boost::asio::placeholders::error));
    }

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
	resetWatchdog();
	readStart();
    }
}

void
IoHandler::resetWatchdog()
{
    m_watchdog.expires_from_now(boost::posix_time::minutes(5));
    m_watchdog.async_wait(boost::bind(&IoHandler::watchdogTimeout, this,
				      boost::asio::placeholders::error));
}

void
IoHandler::watchdogTimeout(const boost::system::error_code& error)
{
    if (error != boost::asio::error::operation_aborted) {
	doClose(error);
    }
}

void
IoHandler::readComplete(const boost::system::error_code& error,
			size_t bytesTransferred)
{
    size_t pos = 0;
    DebugStream& debug = Options::ioDebug();

    if (error) {
	doClose(error);
	return;
    }

    resetWatchdog();

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
	uint8_t dataByte = m_recvBuffer[pos++];
	ssize_t len;

	switch (m_state) {
	    case StartMarker:
		if (m_pos == 0 && dataByte == 0xff) {
		    m_pos++;
		} else if (m_pos == 1 && dataByte == 0xff) {
		    m_state = Flags;
		} else {
		    m_pos = 0;
		}
		break;
	    case Flags:
		m_data.push_back(dataByte);
		m_state = Type;
		break;
	    case Type:
		len = WmrMessage::packetLengthForType(dataByte);
		if (len > 0) {
		    m_pos = len - 2; /* we already read flags + type */
		    m_state = Data;
		    m_data.push_back(dataByte);
		} else {
		    m_data.clear();
		    m_state = StartMarker;
		    m_pos = 0;
		}
		break;
	    case Data:
		m_data.push_back(dataByte);
		m_pos--;
		if (m_pos == 0) {
		    WmrMessage message(m_data, m_db);
		    if (message.isValid()) {
			message.parse();
		    }
		    m_data.clear();
		    m_state = StartMarker;
		}
		break;
	}
    }

    readStart();
}

void
IoHandler::doClose(const boost::system::error_code& error)
{
    if (error && error != boost::asio::error::operation_aborted) {
	std::cerr << "Error: " << error.message() << std::endl;
    }

    m_socket.close();
    m_active = false;
}
