#ifndef __WMRMESSAGE_H__
#define __WMRMESSAGE_H__

#include <vector>
#include "Database.h"

class WmrMessage
{
    public:
	WmrMessage(const std::vector<uint8_t>& data, Database& db);

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
#if 0
	void printNumberAndAddToDb(size_t offset, size_t size, int divider,
				   const char *name, const char *unit,
				   Database::NumericSensors sensor);
	void printBoolAndAddToDb(int byte, int bit, const char *name,
				 Database::BooleanSensors sensor);
	void printStateAndAddToDb(const std::string& value, const char *name,
				  Database::StateSensors sensor);
#endif

    private:
	Database& m_db;
	bool m_valid;
	uint8_t m_flags;
	uint8_t m_type;
	std::vector<unsigned char> m_data;
};

#endif /* __WMRMESSAGE_H__ */
