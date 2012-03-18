#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include "WmrMessage.h"
#include "Options.h"

#define BYTEFORMAT_HEX \
    "0x" << std::setbase(16) << std::setw(2) << std::setfill('0') << (unsigned int)
#define BYTEFORMAT_DEC \
    std::dec << (unsigned int)

WmrMessage::WmrMessage(const std::vector<uint8_t>& data, Database& db) :
    m_db(db)
{
    m_valid = checkValidityAndCopyData(data);
}

ssize_t
WmrMessage::packetLengthForType(uint8_t type)
{
    switch (type) {
	case msgTypeTempHumidity:
	    return 12;
	case msgTypeRain:
	    return 17;
	case msgTypeAirPressure:
	    return 8;
	case msgTypeWind:
	    return 11;
	case msgTypeUV:
	    return 6;
	case msgTypeDateTime:
	    return 12;
    }

    return -1;
}

bool
WmrMessage::checkValidityAndCopyData(const std::vector<uint8_t>& data)
{
    DebugStream& debug = Options::messageDebug();
    std::vector<uint8_t>::const_iterator iter;
    uint16_t calcChecksum = 0, pktChecksum;
    ssize_t expected;

    /* minimum packet length: flags + type + 2 byte checksum */
    if (data.size() < 4) {
	if (debug) {
	    debug << "Packet too small (" << data.size();
	    debug << " bytes, minimum: 4 bytes)" << std::endl;
	}
	return false;
    }

    for (iter = data.begin(); iter < data.end() - 2; iter++) {
	calcChecksum += *iter;
    }
    pktChecksum = (data[data.size() - 1] << 8) | data[data.size() - 2];

    if (calcChecksum != pktChecksum) {
	if (debug) {
	    debug << "Checksum mismatch: " << calcChecksum;
	    debug << " vs. " << pktChecksum << std::endl;
	}
	return false;
    }

    m_flags = data[0];
    m_type = data[1];
    m_data.insert(m_data.begin(), data.begin() + 2, data.end() - 2);

    expected = packetLengthForType(m_type);
    if (expected < 0) {
	if (debug) {
	    debug << "Unexpected type " << BYTEFORMAT_HEX m_type << std::endl;
	}
	return false;
    } else {
	expected -= 4; /* flags + type + checksum */
	if (m_data.size() != expected) {
	    if (debug) {
		debug << "Unexpected packet size for type " << BYTEFORMAT_HEX m_type;
		debug << " (" << std::dec << m_data.size();
		debug << " vs. " << expected << ")" << std::endl;
	    }
	    return false;
	}
    }

    return true;
}

void
WmrMessage::parse()
{
    DebugStream& debug = Options::messageDebug();

    if (!m_valid) {
	return;
    }

    if (debug) {
	time_t now = time(NULL);
	struct tm time;

	localtime_r(&now, &time);
	debug << std::dec << "MESSAGE[";
	debug << std::setw(2) << std::setfill('0') << time.tm_mday;
	debug << "." << std::setw(2) << std::setfill('0') << (time.tm_mon + 1);
	debug << "." << (time.tm_year + 1900) << " ";
	debug << std::setw(2) << std::setfill('0') << time.tm_hour;
	debug << ":" << std::setw(2) << std::setfill('0') << time.tm_min;
	debug << ":" << std::setw(2) << std::setfill('0') << time.tm_sec;
	debug << "]: type " << BYTEFORMAT_HEX m_type;
	debug << ", flags " << BYTEFORMAT_HEX m_flags;
	debug << ", data ";
	for (size_t i = 0; i < m_data.size(); i++) {
	    debug << " " << BYTEFORMAT_HEX m_data[i];
	}
	debug << std::endl;
    }

    parseFlags();

    switch (m_type) {
	case msgTypeTempHumidity:
	    parseTemperatureMessage();
	    break;
	case msgTypeRain:
	    parseRainMessage();
	    break;
	case msgTypeAirPressure:
	    parsePressureMessage();
	    break;
	case msgTypeWind:
	    parseWindMessage();
	    break;
	case msgTypeUV:
	    parseUVMessage();
	    break;
	case msgTypeDateTime:
	    parseDateTimeMessage();
	    break;
	default:
	    {
		DebugStream& dataDebug = Options::dataDebug();
		if (dataDebug) {
		    dataDebug << "DATA: Unhandled message received";
		    dataDebug << "(type " << BYTEFORMAT_HEX m_type << ")." << std::endl;
		}
	    }
	    break;
    }
}

#if 0
void
EmsMessage::printNumberAndAddToDb(size_t offset, size_t size, int divider,
				  const char *name, const char *unit,
				  Database::NumericSensors sensor)
{
    int value = 0;
    float floatVal;

    for (size_t i = offset; i < offset + size; i++) {
	value = (value << 8) | m_data[i];
    }

    /* treat values with highest bit set as negative
     * e.g. size = 2, value = 0xfffe -> real value -2
     */
    if (m_data[offset] & 0x80) {
	value = value - (1 << (size * 8));
    }

    floatVal = value;
    if (divider > 1) {
	floatVal /= divider;
    }

    if (Options::dataDebug()) {
	Options::dataDebug() << "DATA: " << name << " = " << floatVal << " " << unit << std::endl;
    }
    if (m_db && sensor != Database::NumericSensorLast) {
	m_db->addSensorValue(sensor, floatVal);
    }
}

void
EmsMessage::printBoolAndAddToDb(int byte, int bit, const char *name,
				Database::BooleanSensors sensor)
{
    bool flagSet = m_data[byte] & (1 << bit);

    if (Options::dataDebug()) {
	Options::dataDebug() << "DATA: " << name << " = "
			     << (flagSet ? "AN" : "AUS") << std::endl;
    }

    if (m_db && sensor != Database::BooleanSensorLast) {
	m_db->addSensorValue(sensor, flagSet);
    }
}

void
EmsMessage::printStateAndAddToDb(const std::string& value, const char *name,
				 Database::StateSensors sensor)
{
    if (Options::dataDebug()) {
	Options::dataDebug() << "DATA: " << name << " = " << value << std::endl;
    }

    if (m_db && sensor != Database::StateSensorLast) {
	m_db->addSensorValue(sensor, value);
    }
}
#endif

void
WmrMessage::parseFlags()
{
    DebugStream& debug = Options::messageDebug();
    bool batteryLow = m_flags & 0x40;

    if (debug) {
	debug << "Battery " << (batteryLow ? "low" : "ok");
    }
    if (m_type == msgTypeDateTime) {
	bool externalPowerMissing = m_flags & 0x80;
	bool dcfOk = m_flags & 0x20;
	if (debug) {
	    if (!externalPowerMissing) {
		debug << ", externally powered";
	    }
	    debug << ", DCF " << (dcfOk ? "" : "not ") << "synchronized";
	}
    }
    if (debug) {
	debug << std::endl;
    }
}

void
WmrMessage::parseTemperatureMessage()
{
    DebugStream& debug = Options::messageDebug();
    unsigned int sensor = m_data[0] & 0xf;
    unsigned int smiley = m_data[0] >> 6;
    unsigned int trend = (m_data[0] >> 4) & 0x3;
    unsigned int temperatureRaw = ((m_data[2] & 0x0f) << 8) + m_data[1];
    bool temperatureNegative = (m_data[2] >> 4) != 0;
    float temperature = (temperatureNegative ? -0.1f : 0.1f) * temperatureRaw;
    unsigned int humidity = m_data[3];
    unsigned int dewPointRaw = ((m_data[5] & 0x0f) << 8) + m_data[4];
    bool dewPointNegative = (m_data[5] >> 4) != 0;
    float dewPoint = (dewPointNegative ? -0.1f : 0.1f) * dewPointRaw;

    if (debug) {
	debug << "Sensor " << sensor << ": temperature " << temperature;
	debug << "°C, dew point " << dewPoint << "°C, humidity ";
	debug << humidity << "%, smiley " << smiley << ", trend " << trend << std::endl;
    }
}

void
WmrMessage::parseRainMessage()
{
    DebugStream& debug = Options::messageDebug();
    float rate = 0.01f * ((m_data[1] << 8) + m_data[0]);
    float thisHour = 0.01f * ((m_data[3] << 8) + m_data[2]);
    float thisDay = 0.01f * ((m_data[5] << 8) + m_data[4]);
    float total = 0.01f * ((m_data[7] << 8) + m_data[6]);

    if (debug) {
	debug << "Rain: rate " << rate << ", this hour " << thisHour;
	debug << ", thisDay " << thisDay << ", total " << total;
	debug << " since " << BYTEFORMAT_DEC m_data[10] << ".";
	debug << BYTEFORMAT_DEC m_data[11] << ".";
	debug << BYTEFORMAT_DEC (2000 + m_data[12]);
	debug << " " << BYTEFORMAT_DEC m_data[9] << ":";
	debug << BYTEFORMAT_DEC m_data[8] << std::endl;
    }
}

void
WmrMessage::parsePressureMessage()
{
    DebugStream& debug = Options::messageDebug();
    unsigned int pressure = ((m_data[1] & 0xf) << 8) + m_data[0];
    unsigned int altPressure = ((m_data[3] & 0xf) << 8) + m_data[2];
    unsigned int forecast = m_data[1] >> 4;
    unsigned int altForecast = m_data[3] >> 4;

    if (debug) {
	debug << std::dec;
	debug << "Pressure: sea level " << pressure;
	debug << " mbar, altitude " << altPressure << " mbar" << std::endl;
	debug << "Forecast: sea level " << forecast;
	debug << ", altitude " << altForecast << std::endl;
    }
}

void
WmrMessage::parseWindMessage()
{
    DebugStream& debug = Options::messageDebug();
    uint8_t direction = m_data[0] & 0x0f;
    float degrees = 360.0f * direction / 16.0f;
    float avgSpeed = 0.1f * ((m_data[4] << 4) + (m_data[3] >> 4));
    float gustSpeed = 0.1f * (((m_data[3] & 0x0f) << 8) + m_data[2]);

    if (debug) {
	debug << "Wind direction " << BYTEFORMAT_HEX direction;
	debug << " -> " << degrees << "°, speed avg. ";
	debug << avgSpeed << " m/s, gust " << gustSpeed << " m/s" << std::endl;
    }
}

void
WmrMessage::parseUVMessage()
{
    DebugStream& debug = Options::dataDebug();
    unsigned int level = m_data[1];

    if (debug) {
	debug << "UV level: " << level << std::endl;
    }
}

void
WmrMessage::parseDateTimeMessage()
{
    DebugStream& debug = Options::dataDebug();

    if (debug) {
	debug << "Date = " << BYTEFORMAT_DEC m_data[4] << ".";
	debug << BYTEFORMAT_DEC m_data[5] << ".";
	debug << BYTEFORMAT_DEC (2000 + m_data[6]) << std::endl;
	debug << "Time = " << BYTEFORMAT_DEC m_data[3] << ":";
	debug << BYTEFORMAT_DEC m_data[2] << std::endl;
    }
}


