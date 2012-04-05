#ifndef __MYSQLDATABASE_H__
#define __MYSQLDATABASE_H__

#include <map>
#include <queue>
#include <mysql++/connection.h>
#include <mysql++/query.h>
#include "Database.h"

class MysqlDatabase : public virtual Database {
    public:
	MysqlDatabase();
	~MysqlDatabase();

    public:
	bool connect(const std::string& server, const std::string& user, const std::string& password);

	virtual void addSensorValue(NumericSensors sensor, float value);
	virtual void addSensorValue(BooleanSensors sensor, bool value);
	virtual void addSensorValue(StateSensors sensor, const std::string& value);

    private:
	bool createTables();
	void createSensorRows();
	bool executeQuery(mysqlpp::Query& query);

    private:
	static const char *dbName;
	static const char *numericTableName;
	static const char *booleanTableName;
	static const char *stateTableName;

	std::map<unsigned int, float> m_numericCache;
	std::map<unsigned int, bool> m_booleanCache;
	std::map<unsigned int, std::string> m_stateCache;
	std::map<unsigned int, mysqlpp::ulonglong> m_lastInsertIds;
	mysqlpp::Connection *m_connection;
};

#endif /* __MYSQLDATABASE_H__ */
