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

/**
 * @brief Print into a fixed buffer, characters not fitting are dropped.
 *
 * Why it exists: Arduino has no Print writing safely into a fixed buffer,
 * with the limit enforced for each character. String and StreamString use
 * the heap, which might fail or fragment when memory is low, just when e.g.
 * a log has to keep working. snprintf() and functions taking a buffer and
 * its size like ctime_r() depend on the callee respecting the size.
 * std::format_to_n() of C++20 would fit, but is not available on every
 * toolchain and adds a lot of code. BoundedWriter closes this gap: the limit
 * is enforced by the writer itself, no heap is used and the callee does not
 * need to know about it.
 *
 * All print functions end up in write(), which checks the space for each
 * character, so nothing can be written behind the end of the buffer. The
 * buffer is always null terminated. Typical uses are:
 *
 * - Composing text without String and the heap, with everything Print
 *   offers: numbers in any base, floats with a given number of decimals,
 *   Printable objects like IPAddress, String and F() strings, and printf()
 *   where the platform provides it.
 *
 *    char line[17];
 *    BoundedWriter out(line, sizeof(line));
 *    out.print(WiFi.localIP());
 *    out.print(' ');
 *    out.print(23.5f, 1);
 *    lcd.print(out.c_str());
 *
 * - Capturing the output of any code writing to a Print, e.g. a printTo()
 *   function, into a buffer instead of Serial.
 *
 * - Handing a buffer to code which shall not be able to write behind its
 *   end, e.g. a callback.
 *
 * overflowed() tells if something has been dropped, clear() allows to reuse
 * the writer for the next text.
 */
class BoundedWriter : public Print {

    public:

        /**
         * @brief Constructor for BoundedWriter.
         * @param buf The buffer to write to.
         * @param size The size of the buffer including the terminator, at
         *        least one.
         */
        BoundedWriter(char *buf, size_t size) :
              Buf(buf)
            , Size(size)
            , Len(0)
            , Overflow(false)
        {
            Buf[0] = 0;
        }

        /**
         * @brief Write a single character, see Print.
         * @param c The character to write.
         * @return 1 if it has been written, 0 if the buffer is full.
         */
        size_t write(uint8_t c) override {
            if (Len + 1 >= Size) {
                Overflow = true;
                return 0;
            }

            Buf[Len++] = c;
            Buf[Len] = 0;
            return 1;
        }

        using Print::write;

        /**
         * @brief Remove the content, e.g. to reuse the writer.
         */
        void clear() {
            Len = 0;
            Overflow = false;
            Buf[0] = 0;
        }

        /**
         * @brief Get the content.
         * @return The null terminated content.
         */
        const char *c_str() const {
            return Buf;
        }

        /**
         * @brief Get the number of characters written.
         * @return The length of the content, excluding the terminator.
         */
        size_t length() const {
            return Len;
        }

        /**
         * @brief Check if characters have been dropped.
         * @return true if the content has been truncated since the
         *         construction or the last clear().
         */
        bool overflowed() const {
            return Overflow;
        }

    private:

        /**
         * @brief The buffer to write to.
         */
        char *Buf;

        /**
         * @brief The size of the buffer including the terminator.
         */
        size_t Size;

        /**
         * @brief The number of characters written.
         */
        size_t Len;

        /**
         * @brief true if characters have been dropped.
         */
        bool Overflow;

};
