/*
 * ir-gateway, build to automate ir remote control commands in smart homes.
 *
 * Copyright (C) 2025 Julian Friedrich
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
 * @brief A ring buffer for storing strings.
 * This class provides a fixed-size buffer to store strings in a circular manner.
 */
class StringRingBuffer 
{
    public:

        /**
         * @brief Constructor for StringRingBuffer.
         * @param size The size of the buffer.
         */
        StringRingBuffer(int size);

        /**
         * @brief Destructor for StringRingBuffer.
         * Cleans up the resources used by the buffer.
         */
        ~StringRingBuffer();

        /**
         * @brief Push a string into the buffer.
         * @param data The string to be pushed into the buffer.
         */
        void push(const String& data);
        
        /**
         * @brief Pop a string from the buffer.
         * @return The string that was popped from the buffer.
         * If the buffer is empty, it returns an empty string.
         */
        String pop();
        
        /**
         * @brief Peek at the next string in the buffer without removing it.
         * @return The next string in the buffer.
         * If the buffer is empty, it returns an empty string.
         */
        String peek(void) const;

        /**
         * @brief Dump the contents of the buffer as a string.
         * @return A string representation of the buffer contents.
         */
        String dump(void) const;

        /**
         * @brief Check if the buffer is empty.
         * @return true if the buffer is empty, false otherwise.
         */
        bool isEmpty() const {
            return itemCount == 0;
        }

        /**
         * @brief Check if the buffer is full.
         * @return true if the buffer is full, false otherwise.
         */
        bool isFull() const {
            return itemCount == bufferSize;
        }

        /**
         * @brief Get the current size of the buffer.
         * @return The number of items currently in the buffer.
         */
        int size() const {
            return itemCount;
        }

    private:

        /**
         * @brief The size of the buffer.
         * This is the maximum number of items that can be stored in the buffer.
         */
        uint8_t bufferSize;

        /**
         * @brief Pointer to the buffer that stores the strings.
         * This is a dynamically allocated array of strings.
         */
        String* buffer;
        
        /**
         * @brief The index of the head of the buffer.
         * This is the position where the next item will be added.
         */
        uint8_t head;
        
        /**
         * @brief The index of the tail of the buffer.
         * This is the position where the next item will be removed from.
         */
        uint8_t tail;
        
        /**
         * @brief The current number of items in the buffer.
         * This is used to keep track of how many items are currently stored.
         */
        uint8_t itemCount;
};
