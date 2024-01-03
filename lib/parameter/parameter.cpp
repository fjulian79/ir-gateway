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

#include <Arduino.h>
#include "parameter.hpp"

#define readStringParameter(_param_)        readString().toCharArray(_param_, sizeof(_param_))

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

String readString(bool secret = false)
{
    String ret;
    char c = 0;

    while(1)
    {
        while(!Serial.available());
        c = Serial.read();    

        if (c == '\r')
        {
            /* pressed return, we are done */
            Serial.write('\n');
            break;
        }
        else if (c == 0x7f || c == 0x08)
        {
            /* pressed delete */ 
            uint32_t len = ret.length();
            if (len > 0)
            {
                ret.remove(len-1);
                Serial.write("\b \b");
            }
            else
            {
                //Serial.write('\a'); 
            }
        }
        else
        {
            Serial.write(secret ? '*': c);
            ret += (char) c;
        }        
    }

    return ret;
}

int8_t param_clear(void)
{
    Parameter.clear();
        
    sprintf(Parameter.data.ip.hostname, "ir-gateway");
    Parameter.data.ip.dhcp = true;
    sprintf(Parameter.data.ntp.server, "pool.ntp.org");
    /* Vienna shall be the default */
    sprintf(Parameter.data.ntp.timezone, "CET-1CEST,M3.5.0,M10.5.0/3");
    
    Serial.printf("Parameter cleared and set to initial defaults.\n");

    return 0;
}

int8_t param_write(void)
{
    Serial.printf("Enter Parameters in the following order:\n");
    Serial.printf("%s", parameterNames);
    
    readStringParameter(Parameter.data.wifi.ssid);
    readStringParameter(Parameter.data.wifi.pass);
    readStringParameter(Parameter.data.ip.hostname);
    Parameter.data.ip.dhcp = readString().equals("true");
    readStringParameter(Parameter.data.ip.ipaddr);
    readStringParameter(Parameter.data.ip.netmask);
    readStringParameter(Parameter.data.ip.gateway);
    readStringParameter(Parameter.data.ntp.server);
    readStringParameter(Parameter.data.ntp.timezone);
    
    return 0;
}

int8_t param_set(const char *pName)
{
    if(pName == 0)
    {
        Serial.printf("Error, no parameter name given. Valid names are:\n");
        Serial.printf("%s", parameterNames);
        return 0;
    }

    if (strcmp(pName, "ssid") == 0)
    {
        Serial.printf("Enter WiFi SSID: ");
        readStringParameter(Parameter.data.wifi.ssid);
        return 0;
    }

    if (strcmp(pName, "wifi-passwd") == 0)
    {
        Serial.printf("Enter WiFi password: ");
        readString(true).toCharArray(Parameter.data.wifi.pass, 
                sizeof(Parameter.data.wifi.pass));
        return 0;
    }

    if (strcmp(pName, "hostname") == 0)
    {
        Serial.printf("Enter hostname: ");
        readStringParameter(Parameter.data.ip.hostname);
        return 0;
    }

    if (strcmp(pName, "dhcp") == 0)
    {     
        Serial.printf("Enable DHCP? [yes|no]: ");
        Parameter.data.ip.dhcp = readString().equals("yes");
        return 0;
    }

    if (strcmp(pName, "ipaddr") == 0)
    {
        Serial.printf("Enter IPv4 address: ");
        readStringParameter(Parameter.data.ip.ipaddr);
        return 0;
    }

    if (strcmp(pName, "netmask") == 0)
    {
        Serial.printf("Enter IPv4 netmask: ");
        readStringParameter(Parameter.data.ip.netmask);
        return 0;
    }

    if (strcmp(pName, "gateway") == 0)
    {
        Serial.printf("Enter IPv4 gateway: ");
        readStringParameter(Parameter.data.ip.gateway);

        return 0;
    }

    if (strcmp(pName, "ntp-server") == 0)
    {
        Serial.printf("Enter IPv4 ntp server adress: ");
        readStringParameter(Parameter.data.ntp.server);
    
        return 0;
    }

    if (strcmp(pName, "timezone") == 0)
    {
        Serial.printf("Enter the timezone, see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for your zone: ");
        readStringParameter(Parameter.data.ntp.timezone);
        return 0;
    }

    Serial.printf("Error: Invalid parameter!\n");

    return -1;
}

CLI_COMMAND(param)
{
    if (strcmp(argv[0], "clear") == 0)
    {
        return param_clear();
    }

    if (strcmp(argv[0], "write") == 0)
    {
        return param_write();
    }

    if (strcmp(argv[0], "save") == 0)
    {
        Parameter.write();
        Serial.printf("Parameter saved\n");
        return 0;
    }

    if (strcmp(argv[0], "set") == 0)
    {
        return param_set(argv[1]);
    }

    Serial.printf("Error: Invalid command!\n");
    return -1;
}
