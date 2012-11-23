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
