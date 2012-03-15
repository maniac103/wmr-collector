#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <map>
#include <queue>
#include <mysql++/connection.h>
#include <mysql++/query.h>

class Database {
    public:
	Database();
	~Database();

    public:
	bool connect(const std::string& server, const std::string& user, const std::string& password);

    public:
	typedef enum {
	    /* not valid for DB */
	    NumericSensorLast = 24
	} NumericSensors;

	typedef enum {
	    /* not valid for DB */
	    BooleanSensorLast = 124
	} BooleanSensors;

	typedef enum {
	    /* not valid for DB */
	    StateSensorLast = 202
	} StateSensors;

	void addSensorValue(NumericSensors sensor, float value);
	void addSensorValue(BooleanSensors sensor, bool value);
	void addSensorValue(StateSensors sensor, const std::string& value);

    private:
	bool createTables();
	void createSensorRows();
	bool checkAndUpdateRateLimit(unsigned int sensor, time_t now);
	bool executeQuery(mysqlpp::Query& query);

    private:
	static const char *dbName;
	static const char *numericTableName;
	static const char *booleanTableName;
	static const char *stateTableName;

	static const unsigned int sensorTypeNumeric = 1;
	static const unsigned int sensorTypeBoolean = 2;
	static const unsigned int sensorTypeState = 3;

	static const unsigned int readingTypeNone = 0;
	static const unsigned int readingTypeTemperature = 1;
	static const unsigned int readingTypePercent = 2;
	static const unsigned int readingTypeCurrent = 3;
	static const unsigned int readingTypePressure = 4;
	static const unsigned int readingTypeTime = 5;
	static const unsigned int readingTypeCount = 6;

	std::map<unsigned int, time_t> m_lastWrites;
	std::map<unsigned int, float> m_numericCache;
	std::map<unsigned int, bool> m_booleanCache;
	std::map<unsigned int, std::string> m_stateCache;
	std::map<unsigned int, mysqlpp::ulonglong> m_lastInsertIds;
	mysqlpp::Connection *m_connection;
};

#endif /* __DATABASE_H__ */
