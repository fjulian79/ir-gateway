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
 * @brief Maximum length of a message including the terminator, without the
 * timestamp and the severity, longer messages are truncated.
 */
#define SYSLOG_LINE_MAX                 128

#ifndef SYSLOG_LOCALTIME
/**
 * @brief Set to 0 if the local time is never available, e.g. on a device
 * without network. The timestamp is always the time since boot then and the
 * time zone handling of the C library is not needed.
 */
#define SYSLOG_LOCALTIME                1
#endif

/**
 * @brief Management data of a SysLog, stored in front of the lines.
 */
typedef struct {
    uint32_t magic;
    uint32_t size;
    RingBuffer::State state;
} SysLogHeader_t;

/**
 * @brief The memory of a SysLog, the management data and the lines.
 * Created by the user, which decides about the size and where it is placed.
 * @tparam N The size of the lines in bytes.
 */
template <size_t N> struct SysLogMemory {
    SysLogHeader_t header;
    char data[N];
};

/**
 * @brief System log class.
 * Prefixes each message with a timestamp and its severity, keeps the most
 * recent ones in a buffer and prints the important ones to a console:
 *
 *  2026-10-26 18:02:11.417 warning WiFi: Connection lost
 *
 * The timestamp is taken from the system time, which starts at 1970-01-01 on
 * boot, so it is the time since boot until the time has been set, e.g. by
 * ntp. The local time is used from then on, see SYSLOG_LOCALTIME.
 *
 * The severities are the ones of syslog. Messages up to the log level are
 * stored, the ones up to the console level are printed to the console as
 * well. Everything printed to the console is stored too, so the console level
 * is never less severe than the log level. All messages are formatted, also
 * the ones neither stored nor printed, to keep the timing independent of the
 * levels as far as possible. Printing to the console blocks once its buffer
 * is full and changes the timing anyway.
 *
 * To keep the lines across a reset, place the memory where it is not
 * initialized on startup and restore it by begin(), e.g. on an ESP32:
 *
 *  __NOINIT_ATTR static SysLogMemory<8 * 1024> logMemory;
 *  SysLog sysLog(logMemory);
 *  ...
 *  sysLog.begin(esp_reset_reason() != ESP_RST_POWERON);
 *
 * Not safe for concurrent use, only use it from the loop task. E.g. callbacks
 * of the WiFi stack or sntp run in other tasks.
 */
class SysLog {

    public:

        /**
         * @brief The severity of a message, as defined by syslog. The lower
         * the value, the more severe the message.
         */
        typedef enum : uint8_t {
            Emerg = 0,
            Alert,
            Crit,
            Err,
            Warning,
            Notice,
            Info,
            Debug
        } Severity;

        /**
         * @brief Constructor for SysLog without memory.
         * The messages are only printed to the console, but not stored.
         */
        SysLog();

        /**
         * @brief Constructor for SysLog.
         * @param memory The memory to use, see SysLogMemory.
         */
        template <size_t N> SysLog(SysLogMemory<N> &memory) :
            SysLog(&memory.header, memory.data, N)
        {

        }

        /**
         * @brief Constructor for SysLog.
         * @param header The management data.
         * @param data The memory of the lines.
         * @param size The size of data in bytes.
         */
        SysLog(SysLogHeader_t *header, char *data, size_t size);

        /**
         * @brief Restore the lines of the last boot, if requested and valid,
         * clear them otherwise.
         * Has to be called before anything is logged.
         * @param restore true to keep the lines found in the memory, e.g.
         *        after a reset. The memory content is checked, so random
         *        content after a power on is detected as well.
         * @param console Where to print the messages up to the console
         *        level, nullptr for none.
         */
        void begin(bool restore, Print *console = &Serial);

        /**
         * @brief Set the console.
         * @param console Where to print the messages up to the console
         *        level, nullptr for none.
         */
        void setConsole(Print *console);

        /**
         * @brief Set the log level, Info by default.
         * The console level is lowered as well if needed, see SysLog.
         * @param level Messages up to this severity are stored.
         */
        void setLogLevel(Severity level);

        /**
         * @brief Get the log level.
         * @return Messages up to this severity are stored.
         */
        Severity getLogLevel() const;

        /**
         * @brief Set the console level, Err by default.
         * The log level is raised as well if needed, see SysLog.
         * @param level Messages up to this severity are printed to the
         *        console.
         */
        void setConsoleLevel(Severity level);

        /**
         * @brief Get the console level.
         * @return Messages up to this severity are printed to the console.
         */
        Severity getConsoleLevel() const;

        /**
         * @brief Get the name of a severity as used in the log.
         * @param severity The severity.
         * @return The name, e.g. "warning".
         */
        static const char *getSeverityName(Severity severity);

        /**
         * @brief Log a message, printf like.
         * @param severity The severity of the message.
         * @param format The format of the message, without newline.
         */
        void log(Severity severity, const char *format, ...) __attribute__((format(printf, 3, 4)));

        /**
         * @brief Log a message, vprintf like.
         * @param severity The severity of the message.
         * @param format The format of the message, without newline.
         * @param args The arguments of the format.
         */
        void vlog(Severity severity, const char *format, va_list args);

        /**
         * @brief Log a message of the respective severity, printf like.
         * @param format The format of the message, without newline.
         */
        void emerg(const char *format, ...) __attribute__((format(printf, 2, 3)));
        void alert(const char *format, ...) __attribute__((format(printf, 2, 3)));
        void crit(const char *format, ...) __attribute__((format(printf, 2, 3)));
        void err(const char *format, ...) __attribute__((format(printf, 2, 3)));
        void warning(const char *format, ...) __attribute__((format(printf, 2, 3)));
        void notice(const char *format, ...) __attribute__((format(printf, 2, 3)));
        void info(const char *format, ...) __attribute__((format(printf, 2, 3)));
        void debug(const char *format, ...) __attribute__((format(printf, 2, 3)));

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
         * @brief Check the restored lines.
         * @return true if all lines are terminated and fit into an entry.
         */
        bool isValid() const;

        /**
         * @brief The management data, stored with the lines, nullptr if
         * there is no memory.
         */
        SysLogHeader_t *Header;

        /**
         * @brief The buffer holding the lines.
         */
        LineRingBuffer Lines;

        /**
         * @brief Where to print the messages up to the console level.
         */
        Print *Console;

        /**
         * @brief Messages up to this severity are stored.
         */
        Severity LogLevel;

        /**
         * @brief Messages up to this severity are printed to the console.
         */
        Severity ConsoleLevel;

};

/**
 * @brief The system log of the device, defined in main.cpp.
 */
extern SysLog sysLog;
