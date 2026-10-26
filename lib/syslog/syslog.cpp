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
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

/**
 * @brief Size of a stored line, the timestamp, the severity and the message.
 */
#define SYSLOG_ENTRY_SIZE               (SYSLOG_LINE_MAX + 32)

/**
 * @brief Marks the memory as valid, e.g. it is random after power on.
 */
#define SYSLOG_MAGIC                    0x534C4F47u

SysLog::SysLog() :
      Header(nullptr)
    , Lines()
    , Console(&Serial)
    , LogLevel(Info)
    , ConsoleLevel(Err)
{

}

SysLog::SysLog(SysLogHeader_t *header, char *data, size_t size) :
      Header(header)
    , Lines(data, size)
    , Console(&Serial)
    , LogLevel(Info)
    , ConsoleLevel(Err)
{

}

void SysLog::begin(bool restore, Print *console) {
    Console = console;

    if (Header == nullptr) {
        return;
    }

    /* The header is kept with the lines, so a new firmware either finds all
     * of it or none of it at the same place, the size has to match as well */
    if (!restore || Header->magic != SYSLOG_MAGIC || Header->size != Lines.getSize()
            || !Lines.setState(Header->state) || !isValid()) {
        Lines.clear();
        Header->magic = SYSLOG_MAGIC;
        Header->size = Lines.getSize();
        Header->state = Lines.getState();
    }
}

bool SysLog::isValid() const {
    char entry[SYSLOG_ENTRY_SIZE];
    size_t sum = 0;
    size_t len;

    /* A reset while appending a line might leave a line without terminator or
     * longer than an entry, printTo() and getLength() depend on both */
    for (size_t pos = Lines.first(); pos != LineRingBuffer::npos; pos = Lines.next(pos)) {
        len = Lines.read(pos, entry, sizeof(entry));
        if (len == 0) {
            return false;
        }
        sum += len + 1;
    }

    return sum == Lines.getUsed();
}

void SysLog::setConsole(Print *console) {
    Console = console;
}

void SysLog::setLogLevel(Severity level) {
    LogLevel = level > Debug ? Debug : level;
    if (ConsoleLevel > LogLevel) {
        ConsoleLevel = LogLevel;
    }
}

SysLog::Severity SysLog::getLogLevel() const {
    return LogLevel;
}

void SysLog::setConsoleLevel(Severity level) {
    ConsoleLevel = level > Debug ? Debug : level;
    if (LogLevel < ConsoleLevel) {
        LogLevel = ConsoleLevel;
    }
}

SysLog::Severity SysLog::getConsoleLevel() const {
    return ConsoleLevel;
}

const char *SysLog::getSeverityName(Severity severity) {
    static const char *const names[] = {
        "emerg", "alert", "crit", "err", "warning", "notice", "info", "debug"
    };

    return severity <= Debug ? names[severity] : "?";
}

void SysLog::vlog(Severity severity, const char *format, va_list args) {
    char entry[SYSLOG_ENTRY_SIZE];
    struct timeval tv;
    struct tm timeinfo;
    size_t len;
    int ret;

    /* Do not wait for the time as this would add a blocking delay when ntp
     * is not yet synchronized. This would be fatal for logging! The system
     * time does not wait, it counts from 1970-01-01 on boot until it is set.
     */
    gettimeofday(&tv, nullptr);
    gmtime_r(&tv.tv_sec, &timeinfo);

#if SYSLOG_LOCALTIME
    /* Like getLocalTime() a year before 2016 is treated as not set. The time
     * since boot stays in UTC, the time zone might be set before the time */
    if (timeinfo.tm_year > (2016 - 1900)) {
        localtime_r(&tv.tv_sec, &timeinfo);
    }
#endif

    len = snprintf(entry, sizeof(entry), "%04d-%02d-%02d %02d:%02d:%02d.%03lu %-7s ",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
        (unsigned long) (tv.tv_usec / 1000), getSeverityName(severity));

    /* The message is limited to SYSLOG_LINE_MAX, the rest of the entry is
     * reserved for the timestamp and the severity */
    ret = vsnprintf(entry + len, SYSLOG_LINE_MAX, format, args);
    if (ret > 0) {
        len += (size_t) ret < SYSLOG_LINE_MAX ? (size_t) ret : SYSLOG_LINE_MAX - 1;
    }

    /* A newline at the end is not needed, but tolerated */
    while (entry[len - 1] == '\n' || entry[len - 1] == '\r') {
        entry[--len] = 0;
    }

    /* Formatted in any case to keep the timing independent of the levels */
    if (severity <= LogLevel && Header != nullptr) {
        Lines.append(entry, len);
        Header->state = Lines.getState();
    }

    if (severity <= ConsoleLevel && Console != nullptr) {
        /* No println(), its \r\n shows an empty line with monitor_eol = CR */
        Console->print(entry);
        Console->print('\n');
    }
}

void SysLog::log(Severity severity, const char *format, ...) {
    va_list args;

    va_start(args, format);
    vlog(severity, format, args);
    va_end(args);
}

#define SYSLOG_SEVERITY_FUNCTION(_name_, _severity_)            \
    void SysLog::_name_(const char *format, ...) {              \
        va_list args;                                           \
        va_start(args, format);                                 \
        vlog(_severity_, format, args);                         \
        va_end(args);                                           \
    }

SYSLOG_SEVERITY_FUNCTION(emerg, Emerg)
SYSLOG_SEVERITY_FUNCTION(alert, Alert)
SYSLOG_SEVERITY_FUNCTION(crit, Crit)
SYSLOG_SEVERITY_FUNCTION(err, Err)
SYSLOG_SEVERITY_FUNCTION(warning, Warning)
SYSLOG_SEVERITY_FUNCTION(notice, Notice)
SYSLOG_SEVERITY_FUNCTION(info, Info)
SYSLOG_SEVERITY_FUNCTION(debug, Debug)

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
