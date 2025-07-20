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

#include <param/param.hpp>
#include <cli/cli.hpp>

/**
 * @brief Parameter structure for the ir-gateway.
 * This structure holds all the parameters used by the application.
 */
typedef struct {
    struct {
        char ssid[32];
        char pass[32];
    }wifi;

    struct {
        char hostname[32];
        bool dhcp;
        char ipaddr[16];
        char netmask[16];
        char gateway[16];
    }ip;

    struct {
        char server[16];
        char timezone[32];
    }ntp;

} Parameter_t;

/**
 * @brief Global parameter object.
 * This object is used to access and modify the parameters of the application.
 */
extern Param<Parameter_t> Parameter;

/**
 * @brief Clears the parameters and sets them to default values.
 * 
 * This function resets all parameters to their default values.
 * 
 * @return int8_t Returns 0 on success.
 */
int8_t param_clear(void);
