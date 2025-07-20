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

#include "stringRingBuffer.hpp"

StringRingBuffer::StringRingBuffer(int size) : 
      bufferSize(size)
    , buffer(new String[size])
    , head(0)
    , tail(0)
    , itemCount(0) 
{

}

StringRingBuffer::~StringRingBuffer() {
    delete[] buffer;
}
   
void StringRingBuffer::push(const String& data) {
    if (itemCount == bufferSize) {
        pop();
    }

    buffer[head] = data;
    head = (head+1) % bufferSize;
    itemCount++;
}

String StringRingBuffer::pop() {
    if (itemCount == 0) {
        return String();
    }

    String value = buffer[tail];
    tail = (tail + 1) % bufferSize;
    itemCount--;

    return value;
}

String StringRingBuffer::peek() const {
    String data("none");

    if (itemCount != 0) {
        data = buffer[head-1];
    }

    return data;
}

String StringRingBuffer::dump() const {
    String data("empty\n");
    uint8_t pos = tail;

    if (!isEmpty()) {
        data.clear();

        do {
            data += buffer[pos] + "\n";
            pos = (pos + 1) % bufferSize;

        } while(pos != head);    
    }
    
    return data;
}
