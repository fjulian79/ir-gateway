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
#include <generic/lineringbuffer.hpp>

/**
 * @brief Maximum length of an event including the terminator, without the
 * timestamp, longer events are truncated.
 */
#define EVENTLOG_LINE_MAX               128

/**
 * @brief Maximum length of the timestamp, longer ones are truncated, see
 * SYSLOG_TIMESTAMP_MAX for the details.
 */
#define EVENTLOG_TIMESTAMP_MAX          32

/**
 * @brief Log of events like transmitted or received codes.
 * Prefixes each event with a timestamp and keeps the most recent ones in a
 * buffer, the oldest ones are removed as needed:
 *
 *  2026-10-26 21:30:12.417 NEC 0x77E19869
 *
 * Unlike SysLog there are no severities, all events are of the same kind.
 * Each event can be printed to a monitor as well, e.g. to follow the events
 * live on a console like tail -f.
 *
 *  char txData[2048];
 *  EventLog txLog(txData, sizeof(txData));
 *  txLog.setTimeStamp(timeStampSystem);
 *  txLog.log("NEC 0x%08lX", (unsigned long) code);
 *
 * Not safe for concurrent use, only use it from one task.
 */
class EventLog {

    public:

        /**
         * @brief A function printing the timestamp of an event, see
         * SysLog::TimeStampFunc for the details.
         * @param out Where to print the timestamp to.
         */
        typedef void (*TimeStampFunc)(Print &out);

        /**
         * @brief Constructor for EventLog.
         * @param data The memory of the events.
         * @param size The size of data in bytes.
         */
        EventLog(char *data, size_t size);

        /**
         * @brief Set the function printing the timestamp of each event.
         * @param timeStamp The function, nullptr for no timestamp, the
         *        default.
         */
        void setTimeStamp(TimeStampFunc timeStamp);

        /**
         * @brief Set the monitor, each new event is printed to it as well.
         * @param monitor Where to print new events to, nullptr for none, the
         *        default.
         */
        void setMonitor(Print *monitor);

        /**
         * @brief Get the monitor.
         * @return Where new events are printed to, nullptr for none.
         */
        Print *getMonitor() const;

        /**
         * @brief Log an event, printf like.
         * @param format The format of the event, without newline.
         */
        void log(const char *format, ...) __attribute__((format(printf, 2, 3)));

        /**
         * @brief Get the most recent event.
         * @param buf Where to copy the event to.
         * @param size The size of buf.
         * @return The length of the event, 0 if there is none or buf is too
         *         small.
         */
        size_t readLast(char *buf, size_t size) const;

        /**
         * @brief Print all stored events, oldest first, each terminated by a
         * newline.
         * @param out Where to print the events to.
         */
        void printTo(Print &out) const;

        /**
         * @brief Get the number of bytes printTo() writes.
         * @return The length of all stored events including the newlines.
         */
        size_t getLength() const;

    private:

        /**
         * @brief The buffer holding the events.
         */
        LineRingBuffer Lines;

        /**
         * @brief The function printing the timestamp, nullptr for none.
         */
        TimeStampFunc TimeStamp;

        /**
         * @brief Where to print new events to, nullptr for none.
         */
        Print *Monitor;

};
