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

#include "ircontrol.hpp"
#include <generic/timestamp.hpp>

IRControl::IRControl(uint8_t txPin, uint8_t rxPin)
    : irSend(txPin)
    , irRecv(rxPin)
    , txLog(txLogData, sizeof(txLogData))
    , rxLog(rxLogData, sizeof(rxLogData))
    , numTx(0)
    , numRx(0) {
    txLog.setTimeStamp(timeStampSystem);
    rxLog.setTimeStamp(timeStampSystem);
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
    uint16_t retransmit = repeat;

    if (repeat != 0) {
        txLog.log("%s 0x%lX (repeat %ux)", typeToString(type).c_str(), (unsigned long) code, repeat);
    } else {
        txLog.log("%s 0x%lX", typeToString(type).c_str(), (unsigned long) code);
    }
    numTx++;

    irRecv.pause();
    do {
        irSend.send(type, code, 32, 0);
        if (retransmit > 0) {
            retransmit--;
        }
    } while (retransmit > 0);
    irRecv.resume();
}

void IRControl::handleReceive(void) {
    if (irRecv.decode(&irRxData)) {
        String protocol = typeToString(irRxData.decode_type);
        String hexvalue = resultToHexidecimal(&irRxData);

        irRecv.resume();

        rxLog.log("%s %s", protocol.c_str(), hexvalue.c_str());
        numRx++;
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

/**
 * @brief Get the most recent event of a log as String, "none" if empty.
 */
static String getLast(const EventLog &log) {
    char buf[EVENTLOG_TIMESTAMP_MAX + 1 + EVENTLOG_LINE_MAX];

    return log.readLast(buf, sizeof(buf)) ? String(buf) : String("none");
}

String IRControl::getLastTx(void) const {
    return getLast(txLog);
}

String IRControl::getLastRx(void) const {
    return getLast(rxLog);
}

EventLog &IRControl::getTxLog(void) {
    return txLog;
}

EventLog &IRControl::getRxLog(void) {
    return rxLog;
}