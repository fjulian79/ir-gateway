/*
 * ir-gateway, build to automate ir remote control commands in smart homes.
 *
 * Copyright (C) 2023 Julian Friedrich
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

#ifndef STRINGRINGBUFFER_HPP_
#define STRINGRINGBUFFER_HPP_

#include <Arduino.h>

/**
 * A Class implementing a Ring buffer of Strings including a function to dump them all.
*/
class StringRingBuffer 
{
    public:

        /**
         * Constructor
         */
        StringRingBuffer(int size);

        /**
         * Destructor
         */
        ~StringRingBuffer();

        /**
         * Add a String to the Buffer.
         * Pops the oldest element if needed.
        */
        void push(const String& data);
        
        String pop();
        
        String peek(void);

        String dump(void);

        bool isEmpty() const 
        {
            return itemCount == 0;
        }

        bool isFull() const 
        {
            return itemCount == bufferSize;
        }

        int size() const 
        {
            return itemCount;
        }

    private:

        Stream *pStream;
        
        uint8_t bufferSize;

        String* buffer;
        
        uint8_t head;
        
        uint8_t tail;
        
        uint8_t itemCount;
};

#endif /* STRINGRINGBUFFER_HPP_ */