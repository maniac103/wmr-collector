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

#include <iostream>
#include <mysql++/exceptions.h>
#include <mysql++/query.h>
#include <mysql++/ssqls.h>
#include "MysqlDatabase.h"
#include "Options.h"

const char * MysqlDatabase::dbName = "wmr_data";
const char * MysqlDatabase::numericTableName = "numeric_data";

sql_create_4(NumericSensorValue, 1, 4,
	     mysqlpp::sql_smallint, sensor,
	     mysqlpp::sql_float, value,
	     mysqlpp::sql_datetime, starttime,
	     mysqlpp::sql_datetime, endtime);

MysqlDatabase::MysqlDatabase() :
    Database(),
    m_connection(NULL)
{
}

MysqlDatabase::~MysqlDatabase()
{
    if (m_connection) {
	time_t now = time(NULL);
	mysqlpp::sql_datetime timestamp(now);

	for (auto iter = m_lastInsertIds.begin(); iter != m_lastInsertIds.end(); ++iter) {
	    if (iter->first < NumericSensorLast) {
		updateRowEndTime(numericTableName, iter->second, timestamp);
	    }
	}

	delete m_connection;
    }
}

bool
MysqlDatabase::connect(const std::string& server, const std::string& user, const std::string& password)
{
    bool success = false;

    m_connection = new mysqlpp::Connection();
    m_connection->set_option(new mysqlpp::ReconnectOption(true));

    if (!m_connection->connect(NULL, server.c_str(), user.c_str(), password.c_str())) {
	delete m_connection;
	m_connection = NULL;
	return false;
    }

    NumericSensorValue::table(numericTableName);

    try {
	m_connection->select_db(dbName);
	success = true;
    } catch (mysqlpp::DBSelectionFailed& e) {
	/* DB not yet there, need to create it */
	try {
	    m_connection->create_db(dbName);
	    m_connection->select_db(dbName);
	    success = true;
	} catch (mysqlpp::Exception& e) {
	    std::cerr << "Could not create database: " << e.what() << std::endl;
	}
    }

    if (success) {
	success = createTables();
    }
    if (!success) {
	delete m_connection;
	m_connection = NULL;
    }

    return success;
}

bool
MysqlDatabase::createTables()
{
    try {
	mysqlpp::Query query = m_connection->query();
	
	query << "show tables";

	mysqlpp::StoreQueryResult res = query.store();
	if (res && res.num_rows() > 0) {
	    /* tables already present */
	    return true;
	}

	/* Create sensor list table */
	query << "CREATE TABLE IF NOT EXISTS sensors ("
	      << "  type SMALLINT UNSIGNED NOT NULL, "
	      << "  value_type TINYINT UNSIGNED NOT NULL, "
	      << "  name VARCHAR(100) NOT NULL, "
	      << "  reading_type TINYINT UNSIGNED, "
	      << "  unit VARCHAR(10), "
	      << "  `precision` TINYINT UNSIGNED, "
	      << "  PRIMARY KEY (type)) "
	      << "ENGINE MyISAM CHARACTER SET utf8";
	query.execute();

	/* insert sensor data (id, type, name, unit) */
	createSensorRows();

	/* Create numeric sensor data table */
	query << "CREATE TABLE IF NOT EXISTS " << numericTableName << " ("
	      << "  id INT AUTO_INCREMENT, "
	      << "  sensor SMALLINT UNSIGNED NOT NULL, "
	      << "  value FLOAT NOT NULL, "
	      << "  starttime DATETIME NOT NULL, "
	      << "  endtime DATETIME NOT NULL, "
	      << "  PRIMARY KEY (id), "
	      << "  KEY sensor_starttime (sensor, starttime), "
	      << "  KEY sensor_endtime (sensor, endtime)) "
	      << "ENGINE MyISAM PACK_KEYS 1 ROW_FORMAT DYNAMIC";
	query.execute();
    } catch (const mysqlpp::BadQuery& er) {
	std::cerr << "Query error: " << er.what() << std::endl;
	return false;
    } catch (const mysqlpp::BadConversion& er) {
	std::cerr << "Conversion error: " << er.what() << std::endl
		  << "\tretrieved data size: " << er.retrieved
		  << ", actual size: " << er.actual_size << std::endl;
	return false;
    } catch (const mysqlpp::Exception& er) {
	std::cerr << std::endl << "Error: " << er.what() << std::endl;
	return false;
    }

    return true;
}

void
MysqlDatabase::createSensorRows()
{
    mysqlpp::Query query = m_connection->query();

    query << "insert into sensors values (%0q, %1q, %2q, %3q:reading_type, %4q:unit, %5q:precision)";
    query.parse();
    query.template_defaults["unit"] = mysqlpp::null;
    query.template_defaults["reading_type"] = mysqlpp::null;
    query.template_defaults["precision"] = mysqlpp::null;

    /* Numeric sensors */
    query.execute(SensorTempInside, sensorTypeNumeric, "Temperatur Innen", readingTypeTemperature, "°C", 1);
    query.execute(SensorHumidityInside, sensorTypeNumeric, "Luftfeuchte Innen", readingTypePercent, "%", 1);
    query.execute(SensorDewPointInside, sensorTypeNumeric, "Taupunkt Innen", readingTypeTemperature, "°C", 1);
    query.execute(SensorTempOutsideCh1, sensorTypeNumeric, "Temperatur Außen K1", readingTypeTemperature, "°C", 1);
    query.execute(SensorHumidityOutsideCh1, sensorTypeNumeric, "Luftfeuchte Außen K1", readingTypePercent, "%", 1);
    query.execute(SensorDewPointOutsideCh1, sensorTypeNumeric, "Taupunkt Außen K1", readingTypeTemperature, "°C", 1);
    query.execute(SensorTempOutsideCh2, sensorTypeNumeric, "Temperatur Außen K2", readingTypeTemperature, "°C", 1);
    query.execute(SensorHumidityOutsideCh2, sensorTypeNumeric, "Luftfeuchte Außen K2", readingTypePercent, "%", 1);
    query.execute(SensorDewPointOutsideCh2, sensorTypeNumeric, "Taupunkt Außen K2", readingTypeTemperature, "°C", 1);
    query.execute(SensorTempOutsideCh3, sensorTypeNumeric, "Temperatur Außen K3", readingTypeTemperature, "°C", 1);
    query.execute(SensorHumidityOutsideCh3, sensorTypeNumeric, "Luftfeuchte Außen K3", readingTypePercent, "%", 1);
    query.execute(SensorDewPointOutsideCh3, sensorTypeNumeric, "Taupunkt Außen K3", readingTypeTemperature, "°C", 1);
    query.execute(SensorAirPressure, sensorTypeNumeric, "Luftdruck", readingTypePressure, "hPa", 0);
    query.execute(SensorWindSpeedAvg, sensorTypeNumeric, "Windgeschwindigkeit", readingTypeSpeed, "m/s", 1);
    query.execute(SensorWindSpeedGust, sensorTypeNumeric, "Spitzenwindgeschwindigkeit", readingTypeSpeed, "m/s", 1);
    query.execute(SensorWindDirection, sensorTypeNumeric, "Windrichtung", readingTypeNone, "°", 1);
    query.execute(SensorRainRate, sensorTypeNumeric, "Regenrate", readingTypeVolume, "mm/h", 1);
    query.execute(SensorRainAmount, sensorTypeNumeric, "Regenmenge", readingTypeVolume, "mm", 1);
    query.execute(SensorRainTotalSum, sensorTypeNumeric, "Regensumme", readingTypeVolume, "mm", 1);
}

bool
MysqlDatabase::executeQuery(mysqlpp::Query& query)
{
    try {
	query.execute();
	return true;
    } catch (const mysqlpp::BadQuery& e) {
	std::cerr << "MySQL query error: " << e.what() << std::endl;
    } catch (const mysqlpp::Exception& e) {
	std::cerr << "MySQL exception: " << e.what() << std::endl;
    }

    return false;
}

void
MysqlDatabase::addSensorValue(NumericSensors sensor, float value, time_t normalInterval)
{
    if (sensor == SensorRainAmount) {
	value = convertRainAmountValue(value);
    }

    Database::addSensorValue(sensor, value, normalInterval);

    if (m_connection) {
	time_t now = time(NULL);
	std::map<unsigned int, std::pair<float, time_t> >::iterator cacheIter = m_numericCache.find(sensor);

	if (cacheIter != m_numericCache.end() && (now - cacheIter->second.second) > (2 * normalInterval)) {
	    m_numericCache.erase(cacheIter);
	    cacheIter = m_numericCache.end();
	}

	std::map<unsigned int, mysqlpp::ulonglong>::iterator idIter = m_lastInsertIds.find(sensor);
	bool valueChanged = cacheIter == m_numericCache.end() || cacheIter->second.first != value;
	bool idValid = idIter != m_lastInsertIds.end() && idIter->second != 0;
	mysqlpp::sql_datetime timestamp(now);

	if (idValid) {
	    updateRowEndTime(numericTableName, idIter->second, timestamp);
	}

	if (!isnan(value) && (valueChanged || !idValid)) {
	    mysqlpp::Query query = m_connection->query();
	    NumericSensorValue row(sensor, value, timestamp, timestamp);

	    query.insert(row);
	    if (executeQuery(query)) {
		m_lastInsertIds[sensor] = query.insert_id();
		m_numericCache[sensor] = std::make_pair(value, now);
	    }
	}
    }
}

void
MysqlDatabase::updateRowEndTime(const char *table,
				mysqlpp::ulonglong id,
				mysqlpp::sql_datetime& timestamp)
{
    mysqlpp::Query query = m_connection->query();

    query << "update " << table << " set endtime ='" << timestamp
	  << "' where id = " << id;
    executeQuery(query);
}

