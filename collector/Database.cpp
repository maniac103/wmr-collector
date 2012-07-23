#include "Database.h"

Database::Database() :
    m_lastRainAmount(-1),
    m_lastRainDelta(0),
    m_lastRainAmountUpdateTime(0)
{
}

float
Database::convertRainAmountValue(float value)
{
    time_t now = time(NULL);

    if (m_lastRainAmount < 0) {
	m_lastRainAmount = value;
	m_lastRainDelta = 0;
	m_lastRainAmountUpdateTime = now;
    } else if ((now - m_lastRainAmountUpdateTime) >= rainAmountCollectionTime) {
	m_lastRainDelta = std::max(0.0f, value - m_lastRainAmount);
	m_lastRainAmountUpdateTime += rainAmountCollectionTime;
	m_lastRainAmount = value;
    }

    return m_lastRainDelta;
}
