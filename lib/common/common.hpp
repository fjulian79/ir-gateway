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
#include <version/version.h>

/**
 * Pin definitions
 */
#define WIFILED_PIN         2
#define WIFILED_ON          digitalWrite(LED_BUILTIN, 1)
#define WIFILED_OFF         digitalWrite(LED_BUILTIN, 0)
#define IRTX_PIN            22
#define IRRX_PIN            23

/**
 * @brief Check if the string is a valid 32-bit hex number.
 * @param str The string to check.
 * @param maxlen The maximum length of the string, default is 10.
 * @return true if the string is a valid 32-bit hex number, false otherwise.
 * @note The string must start with "0x" and can contain up to 8 hex digits.
 *       The maximum length includes the "0x" prefix.
 */
bool is32BitHex(const char *str, uint8_t maxlen = 10);

/**
 * @brief Get the current timestamp.
 * @return The current timestamp as a string.
 */
String getTimeStamp(void);

/**
 * @brief Get the version string of the project.
 * @return The version string.
 */
String getVersionString(void);
