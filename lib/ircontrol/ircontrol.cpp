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

#include "ircontrol.hpp"

IRControl::IRControl(uint8_t txPin, uint8_t rxPin, uint8_t logSize) 
    : irSend(txPin)
    , irRecv(rxPin)
    , lastTx(logSize)
    , lastRx(logSize)
    , numTx(0)
    , numRx(0) {
}

void IRControl::begin(void) {
    pinMode(IRRX_PIN, INPUT);
    pinMode(IRTX_PIN, OUTPUT);
    digitalWrite(IRTX_PIN, false);

    irSend.begin();
    irRecv.enableIRIn();
}

int8_t IRControl::transmit(const char* type, const char* code, const char* repeat) {
    decode_type_t irType = decode_type_t::UNKNOWN;
    uint32_t base  = 10;
    uint32_t irCode = 0;
    uint16_t irRepeat = 0;
    
    if (type == nullptr || code == nullptr || repeat == nullptr) {
        return -1;
    }

    irType = stringToIRType(type);
    base  = is32BitHex(code) ? 16 : 10;
    irCode = strtoul(code, nullptr, base);
    irRepeat = constrain(atoi(repeat), 0, 15);

    if (irType == decode_type_t::UNKNOWN) {
        return -2;
    }
    
    transmit(irType, irCode, irRepeat);
    return 0;
}

void IRControl::transmit(decode_type_t type, uint32_t code, uint16_t repeat) {
    String hexcode = String(code, HEX);
    String protocol = typeToString(type);
    String ts = getTimeStamp();

    hexcode.toUpperCase();
    lastTx.push(ts + String("; ") + protocol + String("; 0x") + hexcode);
    numTx++;

    irRecv.pause();
    irSend.send(type, code, 32, repeat);
    irRecv.resume();

    Serial.printf("%s IR TX: %s 0x%0X", ts.c_str(), protocol.c_str(), code);
    if(repeat != 0) {
        Serial.printf(" (repeat %dx)", repeat);
    }      
    Serial.printf("\n");
}

void IRControl::handleReceive(void) {
    if (irRecv.decode(&irRxData)) {
        String ts = getTimeStamp();
        String protocol = typeToString(irRxData.decode_type);
        String hexvalue = resultToHexidecimal(&irRxData);
        
        irRecv.resume();

        lastRx.push(ts + String("; ") + protocol + String("; ") + hexvalue);
        numRx++;
        Serial.printf("%s IR RX: %s %s\n", ts.c_str(), protocol.c_str(), hexvalue.c_str());
    }
}

decode_type_t IRControl::stringToIRType(const char * const str) {
    auto *ptr = reinterpret_cast<const char*>(kAllProtocolNamesStr);
    uint16_t length = strlen(ptr);

    for (uint16_t i = 0; length != 0; i++) {
        if (!strcasecmp(str, ptr)) {
            return (decode_type_t)i;
        }
        ptr += length + 1;
        length = strlen(ptr);
    }

    return decode_type_t::UNKNOWN;
}

uint32_t IRControl::getTxCount(void) const {
    return numTx;
}

uint32_t IRControl::getRxCount(void) const {
    return numRx;
}

String IRControl::getLastTx(void) const {
    return lastTx.peek();
}

String IRControl::getLastRx(void) const {
    return lastRx.peek();
}

String IRControl::getTxLog(void) const {
    return lastTx.dump();
}

String IRControl::getRxLog(void) const {
    return lastRx.dump();
}