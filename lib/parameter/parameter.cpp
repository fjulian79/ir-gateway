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

#include <Arduino.h>
#include "parameter.hpp"

#define readStringParameter(_param_, ...)   readStringInto(ioStream, _param_, sizeof(_param_), ##__VA_ARGS__)

Param<Parameter_t> Parameter;

const char parameterNames[] = 
    "  ssid\n"
    "  wifi-passwd\n"
    "  hostname\n"
    "  dhcp\n"
    "  ipaddr\n"
    "  netmask\n"
    "  gateway\n"
    "  ntp-server\n"
    "  timezone\n";

String readString(Stream &ioStream, bool secret = false) {
    String ret;
    char c = 0;

    while(1) {
        while(!ioStream.available()) {
            /* Waiting for user input must not trigger the loop watchdog.
             * One ms delay to avoid busy waiting */
            feedLoopWDT();
            delay(1);
        }
        c = ioStream.read();

        if (c == '\r') {
            /* pressed return, we are done */
            ioStream.write('\n');
            break;
        } else if (c == 0x7f || c == 0x08) {
            /* pressed delete */ 
            uint32_t len = ret.length();
            if (len > 0) {
                ret.remove(len-1);
                ioStream.write("\b \b");
            } else {
                //ioStream.write('\a'); 
            }
        } else {
            ioStream.write(secret ? '*': c);
            ret += (char) c;
        }        
    }

    return ret;
}

/**
 * @brief Reads a string from the serial console into a parameter.
 * Input not fitting into the parameter is rejected instead of truncated, as
 * e.g. a truncated WiFi password would silently prevent connecting.
 * @return true if the parameter has been updated.
 */
bool readStringInto(Stream &ioStream, char *param, size_t size, bool secret = false) {
    String value = readString(ioStream, secret);

    if (value.length() >= size) {
        ioStream.printf("Error: Too long, max %u characters. Value not changed.\n", size - 1);
        return false;
    }

    value.toCharArray(param, size);
    return true;
}

/**
 * @brief Reads a yes/no value from the serial console.
 * @return true for "yes", "true", "on" or "1", false otherwise.
 */
bool readBool(Stream &ioStream) {
    String value = readString(ioStream);

    value.toLowerCase();
    return value == "yes" || value == "true" || value == "on" || value == "1";
}

int8_t param_clear(Stream &ioStream) {
    Parameter.clear();
        
    sprintf(Parameter.data.ip.hostname, "ir-gateway");
    Parameter.data.ip.dhcp = true;
    sprintf(Parameter.data.ntp.server, "pool.ntp.org");
    /* Vienna shall be the default */
    sprintf(Parameter.data.ntp.timezone, "CET-1CEST,M3.5.0,M10.5.0/3");
    
    ioStream.printf("Parameter cleared and set to initial defaults.\n");
    return 0;
}

int8_t param_write(Stream &ioStream) {
    ioStream.printf("Enter Parameters in the following order:\n");
    ioStream.printf("%s", parameterNames);
    
    readStringParameter(Parameter.data.wifi.ssid);
    readStringParameter(Parameter.data.wifi.pass);
    readStringParameter(Parameter.data.ip.hostname);
    Parameter.data.ip.dhcp = readBool(ioStream);
    readStringParameter(Parameter.data.ip.ipaddr);
    readStringParameter(Parameter.data.ip.netmask);
    readStringParameter(Parameter.data.ip.gateway);
    readStringParameter(Parameter.data.ntp.server);
    readStringParameter(Parameter.data.ntp.timezone);
    return 0;
}

int8_t param_set(Stream &ioStream, const char *pName) {
    if(pName == 0) {
        ioStream.printf("Error, no parameter name given. Valid names are:\n");
        ioStream.printf("%s", parameterNames);
        return 0;
    }

    if (strcmp(pName, "ssid") == 0) {
        ioStream.printf("Enter WiFi SSID: ");
        readStringParameter(Parameter.data.wifi.ssid);
        return 0;
    }

    if (strcmp(pName, "wifi-passwd") == 0) {
        ioStream.printf("Enter WiFi password: ");
        readStringParameter(Parameter.data.wifi.pass, true);
        return 0;
    }

    if (strcmp(pName, "hostname") == 0) {
        ioStream.printf("Enter hostname: ");
        readStringParameter(Parameter.data.ip.hostname);
        return 0;
    }

    if (strcmp(pName, "dhcp") == 0) {
        ioStream.printf("Enable DHCP? [yes|no]: ");
        Parameter.data.ip.dhcp = readBool(ioStream);
        return 0;
    }

    if (strcmp(pName, "ipaddr") == 0) {
        ioStream.printf("Enter IPv4 address: ");
        readStringParameter(Parameter.data.ip.ipaddr);
        return 0;
    }

    if (strcmp(pName, "netmask") == 0) {
        ioStream.printf("Enter IPv4 netmask: ");
        readStringParameter(Parameter.data.ip.netmask);
        return 0;
    }

    if (strcmp(pName, "gateway") == 0) {
        ioStream.printf("Enter IPv4 gateway: ");
        readStringParameter(Parameter.data.ip.gateway);

        return 0;
    }

    if (strcmp(pName, "ntp-server") == 0) {
        ioStream.printf("Enter NTP server, hostname or IPv4 address: ");
        readStringParameter(Parameter.data.ntp.server);
    
        return 0;
    }

    if (strcmp(pName, "timezone") == 0) {
        ioStream.printf("Enter the timezone, see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for your zone: ");
        readStringParameter(Parameter.data.ntp.timezone);
        return 0;
    }

    ioStream.printf("Error: Invalid parameter!\n");

    return -1;
}

CLI_COMMAND(param) {
    if (argc == 0) {
        ioStream.printf("Error: Missing command, see help.\n");
        return -1;
    }

    if (strcmp(argv[0], "clear") == 0) {
        return param_clear(ioStream);
    }

    if (strcmp(argv[0], "write") == 0) {
        return param_write(ioStream);
    }

    if (strcmp(argv[0], "save") == 0) {
        Parameter.write();
        ioStream.printf("Parameter saved\n");
        return 0;
    }

    if (strcmp(argv[0], "set") == 0) {
        return param_set(ioStream, argc > 1 ? argv[1] : nullptr);
    }

    ioStream.printf("Error: Invalid command!\n");
    return -1;
}
