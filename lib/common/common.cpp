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

#include "common.hpp"

bool is32BitHex(const char *str, uint8_t maxlen) {
    size_t len = strlen(str);
    if (len > maxlen) {
        return false;
    }

    if (str[0] != '0' || tolower(str[1]) != 'x') {
        return false;
    }

    for (size_t i = 2; i < len; i++) {
        if (!isxdigit(str[i])) {
            return false;
        }
    }

    return true;
}

String getTimeStamp(void) {
    struct tm timeinfo;
    char timeStringBuff[20];

    if (getLocalTime(&timeinfo)) {
        strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
    } else {
        sprintf(timeStringBuff, "ntp error");
    }

    return String(timeStringBuff);
}

String getVersionString(void) {
    String data = String(VERSION_PROJECT) + " " + String(VERSION_GIT_SHORT) + ", Copyright (C) 2024 Julian Friedrich\n";

    data += "Build:    " + String(__DATE__) + ", " + __TIME__ + "\n";
    data += "Git Repo: " + String(VERSION_GIT_REMOTE_ORIGIN) + "\n";
    data += "Revision: " + String(VERSION_GIT_LONG) + "\n";
    data += "\n";
    data += "This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n";
    data += "are welcome to redistribute it under certain conditions.\n";
    data += "See GPL v3 licence at https://www.gnu.org/licenses/ for details.\n";
    
    return data;
}