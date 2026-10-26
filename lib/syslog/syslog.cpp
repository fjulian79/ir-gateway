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

#include "syslog.hpp"
#include <esp_timer.h>

/**
 * @brief Size of a stored line, the timestamp and the line.
 */
#define SYSLOG_ENTRY_SIZE               (SYSLOG_LINE_MAX + 24)

static char logData[SYSLOG_SIZE];

SysLog sysLog;

SysLog::SysLog() :
      Lines(logData, sizeof(logData))
    , LineLen(0)
{

}

size_t SysLog::write(uint8_t c) {
    if (c == '\n') {
        flushLine();
    } else if (c != '\r' && c != '\0') {
        if (LineLen == sizeof(Line) - 1) {
            flushLine();
        }
        Line[LineLen++] = c;
    }

    return 1;
}

size_t SysLog::write(const uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        write(buffer[i]);
    }

    return size;
}

void SysLog::flushLine() {
    char entry[SYSLOG_ENTRY_SIZE];
    struct tm timeinfo;
    size_t len;

    /* Do not wait for the time as this would add a blocking delay when ntp
     * is not yet synchronized. This would be fatal for logging!
     * If we dont have ntp time, we fall back to using the system uptime.
     */
    if (getLocalTime(&timeinfo, 0)) {
        len = strftime(entry, sizeof(entry), "%Y-%m-%d %H:%M:%S ", &timeinfo);
    } else {
        /* 64 bit as millis() wraps after 49 days */
        uint32_t s = esp_timer_get_time() / 1000000;
        len = snprintf(entry, sizeof(entry), "%3lud %02lu:%02lu:%02lu ", (unsigned long) (s / 86400),
            (unsigned long) (s / 3600 % 24), (unsigned long) (s / 60 % 60), (unsigned long) (s % 60));
    }

    memcpy(entry + len, Line, LineLen);
    len += LineLen;
    entry[len] = 0;
    LineLen = 0;

    Lines.append(entry, len);
    Serial.println(entry);
}

void SysLog::printTo(Print &out) const {
    char entry[SYSLOG_ENTRY_SIZE];
    size_t len;

    for (size_t pos = Lines.first(); pos != LineRingBuffer::npos; pos = Lines.next(pos)) {
        len = Lines.read(pos, entry, sizeof(entry));
        entry[len] = '\n';
        out.write((const uint8_t *) entry, len + 1);
    }
}

size_t SysLog::getLength() const {
    /* Each line is stored with a null terminator, printed as newline */
    return Lines.getUsed();
}
