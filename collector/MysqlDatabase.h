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

#ifndef __MYSQLDATABASE_H__
#define __MYSQLDATABASE_H__

#include <map>
#include <queue>
#include <mysql++/mysql++.h>
#include <mysql++/connection.h>
#include <mysql++/query.h>
#include "Database.h"

class MysqlDatabase : public virtual Database {
    public:
	MysqlDatabase();
	virtual ~MysqlDatabase();

    public:
	bool connect(const std::string& server, const std::string& user, const std::string& password);

	virtual void addSensorValue(NumericSensors sensor, float value, time_t normalInterval);

    private:
	bool createTables();
	void createSensorRows();
	bool executeQuery(mysqlpp::Query& query);
	void updateRowEndTime(const char *table, mysqlpp::ulonglong id, mysqlpp::sql_datetime& timestamp);

    private:
	static const char *dbName;
	static const char *numericTableName;

	std::map<unsigned int, std::pair<float, time_t> > m_numericCache;
	std::map<unsigned int, mysqlpp::ulonglong> m_lastInsertIds;
	mysqlpp::Connection *m_connection;
};

#endif /* __MYSQLDATABASE_H__ */
