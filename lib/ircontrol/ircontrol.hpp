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
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

#include "common.hpp"
#include "eventlog.hpp"

/**
 * @brief Size of the transmission and the reception log in bytes each.
 */
#define IRCONTROL_LOG_SIZE              2048

/**
 * @brief Class to handle IR control functionality.
 * This class provides methods to transmit and receive IR signals,
 * as well as manage the transmission and reception logs.
 */
class IRControl {
    public:
        
        /**
         * Constructor
         * @param txPin Pin for IR transmission, default is IRTX_PIN.
         * @param rxPin Pin for IR reception, default is IRRX_PIN.
         */
        IRControl(uint8_t txPin = IRTX_PIN, uint8_t rxPin = IRRX_PIN);
        
        /**
         * @brief Initialize the IR control.
         * This method sets up the IR send and receive functionality.
         */
        void begin(void);
        
        /**
         * @brief Transmit an IR signal.
         * @param type The type of IR protocol to use.
         * @param code The code to transmit.
         * @param repeat The number of times to repeat the transmission.
         * @return 0 on success, negative value on error.
         */
        int8_t transmit(const char* type, const char* code, const char* repeat);

        /**
         * @brief Transmit an IR signal.
         * @param type The type of IR protocol to use.
         * @param code The code to transmit.
         * @param repeat The number of times to repeat the transmission, default is 0.
         */
        void transmit(decode_type_t type, uint32_t code, uint16_t repeat = 0);
        
        /**
         * @brief Handle the reception of IR signals.
         * This method processes incoming IR signals and updates the reception log.
         */
        void handleReceive(void);
        
        /**
         * @brief Convert a string to an IR type.
         * @param str The string to convert.
         * @return The corresponding decode_type_t enum value.
         */
        decode_type_t stringToIRType(const char * const str);
        
        /**
         * @brief Get the number of transmitted IR signals.
         * @return The number of transmitted IR signals.
         */
        uint32_t getTxCount(void) const;
        
        /**
         * @brief Get the number of received IR signals.
         * @return The number of received IR signals.
         */
        uint32_t getRxCount(void) const;
        
        /**
         * @brief Get the last transmitted IR signal.
         * @return A string containing the last transmitted IR signal.
         */
        String getLastTx(void) const;
        
        /**
         * @brief Get the last received IR signal.
         * @return A string containing the last received IR signal.
         */
        String getLastRx(void) const;
        
        /**
         * @brief Get the transmission log.
         * @return The log of the transmitted IR signals.
         */
        EventLog &getTxLog(void);

        /**
         * @brief Get the reception log.
         * @return The log of the received IR signals.
         */
        EventLog &getRxLog(void);

    private:
        
        /**
         * IR send object.
         */ 
        IRsend irSend;
        
        /**
         * IR receive object.
         */
        IRrecv irRecv;
        
        /**
         * IR receive data structure.
         */
        decode_results irRxData;
        
        /**
         * Memory of the transmission log.
         */
        char txLogData[IRCONTROL_LOG_SIZE];

        /**
         * Memory of the reception log.
         */
        char rxLogData[IRCONTROL_LOG_SIZE];

        /**
         * Transmission log.
         */
        EventLog txLog;

        /**
         * Reception log.
         */
        EventLog rxLog;
        
        /**
         * Number of transmitted IR signals.
         */
        uint32_t numTx;
        
        /**
         * Number of received IR signals.
         */
        uint32_t numRx;
};
