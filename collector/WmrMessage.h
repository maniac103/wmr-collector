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

#ifndef __WMRMESSAGE_H__
#define __WMRMESSAGE_H__

#include <vector>
#include <boost/shared_ptr.hpp>
#include "Database.h"

class WmrMessage
{
    public:
	WmrMessage(const std::vector<uint8_t>& data, boost::shared_ptr<Database>& db);

	static ssize_t packetLengthForType(uint8_t type);
	bool isValid() const {
	    return m_valid;
	}
	void parse();

    private:
	static const uint8_t msgTypeRain = 0x41;
	static const uint8_t msgTypeTempHumidity = 0x42;
	static const uint8_t msgTypeAirPressure = 0x46;
	static const uint8_t msgTypeUV = 0x47;
	static const uint8_t msgTypeWind = 0x48;
	static const uint8_t msgTypeDateTime = 0x60;

    private:
	bool checkValidityAndCopyData(const std::vector<uint8_t>& data);

	void parseFlags();
	void parseTemperatureMessage();
	void parseRainMessage();
	void parsePressureMessage();
	void parseWindMessage();
	void parseUVMessage();
	void parseDateTimeMessage();

    private:
	boost::shared_ptr<Database> m_db;
	bool m_valid;
	uint8_t m_flags;
	uint8_t m_type;
	std::vector<unsigned char> m_data;
};

#endif /* __WMRMESSAGE_H__ */
