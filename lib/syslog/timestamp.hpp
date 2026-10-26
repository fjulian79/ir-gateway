/*
 * ir-gateway, build to automate ir remote control commands in smart homes.
 *
 * Copyright (C) 2026 Julian Friedrich
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * You can file issues at https://github.com/fjulian79/ir-gateway/issues
 */

#pragma once

#include <Arduino.h>

/*
 * Functions printing a timestamp to any Print, without a separator at the
 * end, e.g. for SysLog::setTimeStamp():
 *
 *  sysLog.setTimeStamp(timeStampSystem);
 *
 * They do not depend on SysLog and can be called directly as well:
 *
 *  timeStampMillis(Serial);
 */

/**
 * @brief Prints the time since boot based on millis() as hh:mm:ss.ms, e.g.
 * "01:02:03.004". Available on every platform, but millis() wraps after 49.7
 * days. The hours are not limited to 24, from 100 hours on they take three
 * digits and the timestamp gets one character longer.
 */
void timeStampMillis(Print &out);

#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
/*
 * The system time of these platforms counts from 1970-01-01 on boot until it
 * is set, e.g. by ntp, so it is the time since boot as long as it has not been
 * set. Other platforms might not provide a system time at all.
 */

/**
 * @brief Prints the local system time as "YYYY-MM-DD hh:mm:ss.ms".
 * As long as the time has not been set it is printed in UTC, so the time
 * since boot does not depend on the time zone, which might be set earlier.
 */
void timeStampSystem(Print &out);
#endif
