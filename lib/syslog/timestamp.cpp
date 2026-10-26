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

#include "timestamp.hpp"

/* Formatted by snprintf() and printed by print(), as not every Print provides
 * printf() */

void timeStampMillis(Print &out) {
    char buf[24];
    uint32_t ms = millis();

    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu.%03lu", (unsigned long) (ms / 3600000),
        (unsigned long) (ms / 60000 % 60), (unsigned long) (ms / 1000 % 60), (unsigned long) (ms % 1000));
    out.print(buf);
}

#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040)

#include <sys/time.h>
#include <time.h>

/**
 * @brief Prints the given time as "YYYY-MM-DD hh:mm:ss.ms".
 */
static void printTime(Print &out, const struct tm &timeinfo, long usec) {
    /* Large enough for any int value of the fields, not only valid times */
    char buf[80];

    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03lu",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, (unsigned long) (usec / 1000));
    out.print(buf);
}

void timeStampSystem(Print &out) {
    struct timeval tv;
    struct tm timeinfo;

    /* Do not wait for the time as this would add a blocking delay when ntp
     * is not yet synchronized. This would be fatal for logging! The system
     * time does not wait, it counts from 1970-01-01 on boot until it is set.
     */
    gettimeofday(&tv, nullptr);
    gmtime_r(&tv.tv_sec, &timeinfo);

    /* Like getLocalTime() a year before 2016 is treated as not set. The time
     * since boot stays in UTC, the time zone might be set before the time */
    if (timeinfo.tm_year > (2016 - 1900)) {
        localtime_r(&tv.tv_sec, &timeinfo);
    }

    printTime(out, timeinfo, tv.tv_usec);
}

#endif
