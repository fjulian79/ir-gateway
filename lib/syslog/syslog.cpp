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
#include "boundedwriter.hpp"
#include <stdarg.h>

/**
 * @brief Width of the severity column, the length of the longest name.
 */
#define SYSLOG_SEVERITY_WIDTH           7

/**
 * @brief Size of a stored line, the timestamp, the severity, the separators
 * and the message.
 */
#define SYSLOG_ENTRY_SIZE               (SYSLOG_TIMESTAMP_MAX + 1 + SYSLOG_SEVERITY_WIDTH + 1 + SYSLOG_LINE_MAX)

/**
 * @brief Marks the memory as valid, e.g. it is random after power on.
 */
#define SYSLOG_MAGIC                    0x534C4F47u

SysLog::SysLog() :
      Header(nullptr)
    , Lines()
    , TimeStampFmtCb(nullptr)
    , Console(&Serial)
    , LogLevel(Info)
    , ConsoleLevel(Err)
{

}

SysLog::SysLog(SysLogHeader_t *header, char *data, size_t size) :
      Header(header)
    , Lines(data, size)
    , TimeStampFmtCb(nullptr)
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

void SysLog::setTimeStamp(TimeStampFunc timeStamp) {
    TimeStampFmtCb = timeStamp;
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
    size_t len = 0;
    int ret;

    if (TimeStampFmtCb != nullptr) {
        /* Set up the bounded writer for the timestamp to the temporary buffer */
        BoundedWriter out(entry, SYSLOG_TIMESTAMP_MAX + 1);
        /* Let the calback print the timestamp by using the bounded writer */
        TimeStampFmtCb(out);
        /* Get the length of the timestamp written by the callback */
        len = out.length();
        if (len > 0) {
            entry[len++] = ' ';
        }
    }

    /* Write the severity after the timestamp while adding padding */
    len += snprintf(entry + len, sizeof(entry) - len, "%-*s ", SYSLOG_SEVERITY_WIDTH,
        getSeverityName(severity));

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
