#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include "WmrMessage.h"
#include "Options.h"

#define HEX(value) \
    "0x" << std::setbase(16) << std::setw(2) << \
    std::setfill('0') << (unsigned int) (value) << std::dec
#define DEC(value) \
    std::dec << (unsigned int) (value)

WmrMessage::WmrMessage(const std::vector<uint8_t>& data, boost::shared_ptr<Database>& db) :
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
	    debug << "Unexpected type " << HEX(m_type) << std::endl;
	}
	return false;
    } else {
	expected -= 4; /* flags + type + checksum */
	if (m_data.size() != (size_t) expected) {
	    if (debug) {
		debug << "Unexpected packet size for type " << HEX(m_type);
		debug << " (" << m_data.size() << " vs. " << expected << ")";
		debug << std::endl;
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
	debug << "MESSAGE[";
	debug << std::setw(2) << std::setfill('0') << time.tm_mday;
	debug << "." << std::setw(2) << std::setfill('0') << (time.tm_mon + 1);
	debug << "." << (time.tm_year + 1900) << " ";
	debug << std::setw(2) << std::setfill('0') << time.tm_hour;
	debug << ":" << std::setw(2) << std::setfill('0') << time.tm_min;
	debug << ":" << std::setw(2) << std::setfill('0') << time.tm_sec;
	debug << "]: type " << HEX(m_type);
	debug << ", flags " << HEX(m_flags);
	debug << ", data ";
	for (size_t i = 0; i < m_data.size(); i++) {
	    debug << " " << HEX(m_data[i]);
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
		    dataDebug << "(type " << HEX(m_type) << ")." << std::endl;
		}
	    }
	    break;
    }
}

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

float
WmrMessage::roundedValueFromRaw(uint8_t highByte, uint8_t lowByte, float factor, bool negative)
{
    unsigned int raw = (highByte << 8) + lowByte;
    return (negative ? -factor : factor) * raw;
}

void
WmrMessage::parseTemperatureMessage()
{
    static const char *trendStrings[] = {
	"steady", "rising", "falling", "???"
    };
    static const char *smileyStrings[] = {
	"   ", ":-)", ":-(", ":-|"
    };

    DebugStream& debug = Options::messageDebug();
    unsigned int sensor = m_data[0] & 0xf;
    unsigned int smiley = m_data[0] >> 6;
    unsigned int trend = (m_data[0] >> 4) & 0x3;
    unsigned int humidity = m_data[3];
    float temperature = roundedValueFromRaw(m_data[2] & 0x0f, m_data[1], 0.1f, (m_data[2] >> 4) != 0);
    float dewPoint = roundedValueFromRaw(m_data[5] & 0x0f, m_data[4], 0.1f, (m_data[5] >> 4) != 0);

    if (debug) {
	debug << "Sensor " << sensor << ": temperature " << temperature;
	debug << "°C, dew point " << dewPoint << "°C, humidity ";
	debug << humidity << "%, trend " << trendStrings[trend];
	debug << ", smiley " << smileyStrings[smiley] << std::endl;
    }

    if (m_db) {
	Database::NumericSensors tempSensor = (sensor == 0) ? Database::SensorTempInside :
					      (sensor == 1) ? Database::SensorTempOutsideCh1 :
					      (sensor == 2) ? Database::SensorTempOutsideCh2 :
					      (sensor == 3) ? Database::SensorTempOutsideCh3 :
					      Database::NumericSensorLast;
	if (tempSensor != Database::NumericSensorLast) {
	    m_db->addSensorValue(tempSensor, temperature);
	}

	Database::NumericSensors dewSensor = (sensor == 0) ? Database::SensorDewPointInside :
					     (sensor == 1) ? Database::SensorDewPointOutsideCh1 :
					     (sensor == 2) ? Database::SensorDewPointOutsideCh2 :
					     (sensor == 3) ? Database::SensorDewPointOutsideCh3 :
					     Database::NumericSensorLast;
	if (dewSensor != Database::NumericSensorLast) {
	    m_db->addSensorValue(dewSensor, dewPoint);
	}

	Database::NumericSensors humidSensor = (sensor == 0) ? Database::SensorHumidityInside :
					       (sensor == 1) ? Database::SensorHumidityOutsideCh1 :
					       (sensor == 2) ? Database::SensorHumidityOutsideCh2 :
					       (sensor == 3) ? Database::SensorHumidityOutsideCh3 :
					       Database::NumericSensorLast;
	if (humidSensor != Database::NumericSensorLast) {
	    m_db->addSensorValue(humidSensor, humidity);
	}
    }
}

void
WmrMessage::parseRainMessage()
{
    DebugStream& debug = Options::messageDebug();
    float rate = roundedValueFromRaw(m_data[1], m_data[0], 0.01f * 25.4f);
    float thisHour = roundedValueFromRaw(m_data[3], m_data[2], 0.01f * 25.4f);
    float thisDay = roundedValueFromRaw(m_data[5], m_data[4], 0.01f * 25.4f);
    float total = roundedValueFromRaw(m_data[7], m_data[6], 0.01f * 25.4f);

    if (debug) {
	debug << "Rain: rate " << rate << ", this hour " << thisHour;
	debug << ", thisDay " << thisDay << ", total " << total;
	debug << " since " << std::setw(2) << std::setfill('0') << DEC(m_data[10]);
	debug << "." << std::setw(2) << std::setfill('0') << DEC(m_data[11]);
	debug << "." << DEC(2000 + m_data[12]) << " ";
	debug << std::setw(2) << std::setfill('0') << DEC(m_data[9]);
	debug << ":" << std::setw(2) << std::setfill('0') << DEC(m_data[8]);
	debug << std::endl;
    }

    if (m_db) {
	m_db->addSensorValue(Database::SensorRainfall, rate);
    }
}

void
WmrMessage::parsePressureMessage()
{
    static const char *forecastStrings[] = {
	"partly cloudy", /* day */	"rainy",
	"cloudy",			"sunny", /* day */
	"clear", /* night */		"snowy",
	"partly cloudy", /* night */	"???",
	"???",				"???",
	"???",				"???",
	"???",				"???",
	"???",				"???"
    };

    DebugStream& debug = Options::messageDebug();
    unsigned int pressure = ((m_data[1] & 0xf) << 8) + m_data[0];
    unsigned int altPressure = ((m_data[3] & 0xf) << 8) + m_data[2];
    unsigned int forecast = m_data[1] >> 4;
    unsigned int altForecast = m_data[3] >> 4;

    if (debug) {
	const char *forecastString = forecastStrings[forecast];
	const char *altForecastString = forecastStrings[altForecast];

	debug << std::dec;
	debug << "Pressure: sea level " << pressure;
	debug << " mbar, altitude " << altPressure << " mbar" << std::endl;
	debug << "Forecast: sea level " << forecastString;
	debug << ", altitude " << altForecastString << std::endl;
    }

    if (m_db) {
	m_db->addSensorValue(Database::SensorAirPressure, altPressure);
    }
}

void
WmrMessage::parseWindMessage()
{
    static const char *directionStrings[] = {
	"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
	"S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };

    DebugStream& debug = Options::messageDebug();
    uint8_t direction = m_data[0] & 0x0f;
    float degrees = 360.0f * direction / 16.0f;
    float avgSpeed = roundedValueFromRaw(m_data[4] << 4, m_data[3] >> 4, 0.1f);
    float gustSpeed = roundedValueFromRaw(m_data[3] & 0x0f, m_data[2], 0.1f);
    float windChill;

    if (m_data[6] != 0x20) {
	windChill = (roundedValueFromRaw(m_data[6] & 0x0f, m_data[5], 0.1f) - 32) / 1.8f;
    } else {
	windChill = NAN;
    }

    if (debug) {
	debug << "Wind direction " << directionStrings[direction];
	debug << " -> " << degrees << "°, speed avg. ";
	debug << avgSpeed << " m/s, gust " << gustSpeed << " m/s" << std::endl;
	debug << "Wind chill temperature ";
	if (isnan(windChill)) {
	    debug << "n/a";
	} else {
	    debug << windChill << " °C";
	}
	debug << std::endl;
    }

    if (m_db) {
	m_db->addSensorValue(Database::SensorWindSpeedAvg, avgSpeed);
	if (avgSpeed != gustSpeed) {
	    m_db->addSensorValue(Database::SensorWindSpeedGust, gustSpeed);
	}
	m_db->addSensorValue(Database::SensorWindDirection, degrees);
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
    if (m_db) {
	m_db->addSensorValue(Database::SensorUVLevel, (float) level);
    }
}

void
WmrMessage::parseDateTimeMessage()
{
    DebugStream& debug = Options::dataDebug();

    if (debug) {
	debug << "Date = " << std::setw(2) << std::setfill('0') << DEC(m_data[4]);
	debug << "." << std::setw(2) << std::setfill('0') << DEC(m_data[5]);
	debug << "." << DEC(2000 + m_data[6]) << std::endl;
	debug << "Time = " << std::setw(2) << std::setfill('0') << DEC(m_data[3]);
	debug << ":" << std::setw(2) << std::setfill('0') << DEC(m_data[2]);
	debug << std::endl;
    }
}
