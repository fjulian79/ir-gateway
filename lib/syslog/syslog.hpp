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
 * @brief Size of the log buffer in bytes.
 */
#define SYSLOG_SIZE                     (8 * 1024)

/**
 * @brief Maximum length of a line without the timestamp, longer lines are
 * split.
 */
#define SYSLOG_LINE_MAX                 128

/**
 * @brief System log class.
 * Collects the written text to lines, prefixes them with a timestamp and
 * keeps the most recent lines in a buffer. Each line is printed to Serial as
 * well. The timestamp is the local time once it has been synchronized, the
 * up time before.
 *
 * Not safe for concurrent use, only use it from the loop task. E.g. callbacks
 * of the WiFi stack or sntp run in other tasks.
 */
class SysLog : public Print {

    public:

        /**
         * @brief Constructor for SysLog.
         */
        SysLog();

        /**
         * @brief Write a single character, see Print.
         * @param c The character to write.
         * @return The number of characters written.
         */
        size_t write(uint8_t c) override;

        /**
         * @brief Write a buffer, see Print.
         * @param buffer The data to write.
         * @param size The number of characters to write.
         * @return The number of characters written.
         */
        size_t write(const uint8_t *buffer, size_t size) override;

        using Print::write;

        /**
         * @brief Print all stored lines, oldest first, each terminated by a
         * newline.
         * @param out Where to print the lines to.
         */
        void printTo(Print &out) const;

        /**
         * @brief Get the number of bytes printTo() writes.
         * @return The length of all stored lines including the newlines.
         */
        size_t getLength() const;

    private:

        /**
         * @brief Store and print the collected line.
         */
        void flushLine();

        /**
         * @brief The buffer holding the lines.
         */
        LineRingBuffer Lines;

        /**
         * @brief The line collected so far.
         */
        char Line[SYSLOG_LINE_MAX];

        /**
         * @brief Length of the collected line.
         */
        size_t LineLen;

};

/**
 * @brief The system log of the device.
 */
extern SysLog sysLog;
