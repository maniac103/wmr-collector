#ifndef __DATABASE_H__
#define __DATABASE_H__

class Database {
    protected:
	Database() { };
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
	    SensorRainfall = 51,
	    /* not valid for DB */
	    NumericSensorLast = 255
	} NumericSensors;

	typedef enum {
	    /* not valid for DB */
	    BooleanSensorLast = 1
	} BooleanSensors;

	typedef enum {
	    /* not valid for DB */
	    StateSensorLast = 1
	} StateSensors;

	virtual void addSensorValue(NumericSensors sensor, float value) {}
	virtual void addSensorValue(BooleanSensors sensor, bool value) {}
	virtual void addSensorValue(StateSensors sensor, const std::string& value) {}

    protected:
	static const unsigned int sensorTypeNumeric = 1;
	static const unsigned int sensorTypeBoolean = 2;
	static const unsigned int sensorTypeState = 3;

	static const unsigned int readingTypeNone = 0;
	static const unsigned int readingTypeTemperature = 1;
	static const unsigned int readingTypePercent = 2;
	static const unsigned int readingTypeSpeed = 3;
	static const unsigned int readingTypePressure = 4;
	static const unsigned int readingTypeVolume = 5;
};

#endif /* __DATABASE_H__ */
