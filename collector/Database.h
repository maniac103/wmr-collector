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

#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <time.h>
#include <string>

class Database {
    protected:
	Database();
    public:
	~Database() { };

    public:
	typedef enum {
	    SensorTempInside = 1,
	    SensorHumidityInside = 2,
	    SensorDewPointInside = 3,
	    SensorTempOutsideCh1 = 11,
	    SensorHumidityOutsideCh1 = 12,
	    SensorDewPointOutsideCh1 = 13,
	    SensorTempOutsideCh2 = 21,
	    SensorHumidityOutsideCh2 = 22,
	    SensorDewPointOutsideCh2 = 23,
	    SensorTempOutsideCh3 = 31,
	    SensorHumidityOutsideCh3 = 32,
	    SensorDewPointOutsideCh3 = 33,
	    SensorAirPressure = 41,
	    SensorUVLevel = 42,
	    SensorWindSpeedAvg = 45,
	    SensorWindSpeedGust = 46,
	    SensorWindDirection = 47,
	    SensorRainRate = 51,
	    SensorRainAmount = 52,
	    SensorRainTotalSum = 53,
	    /* not valid for DB */
	    NumericSensorLast = 512
	} NumericSensors;

	virtual void addSensorValue(NumericSensors sensor, float value, time_t normalInterval) {}

    protected:
	float convertRainAmountValue(float value);

    private:
	float m_lastRainAmount;
	float m_lastRainDelta;
	time_t m_lastRainAmountUpdateTime;

	static const long rainAmountCollectionTime = 15 * 60; /* collect for 15 minutes */

    protected:
	static const unsigned int sensorTypeNumeric = 1;

	static const unsigned int readingTypeNone = 0;
	static const unsigned int readingTypeTemperature = 1;
	static const unsigned int readingTypePercent = 2;
	static const unsigned int readingTypeSpeed = 3;
	static const unsigned int readingTypePressure = 4;
	static const unsigned int readingTypeVolume = 5;
};

#endif /* __DATABASE_H__ */
