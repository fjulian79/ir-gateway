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

#include "webservercontrol.hpp"
#include <WiFi.h>
#include "parameter.hpp"
#include "ircontrol.hpp"
#include <generic/uptime.hpp>
#include <version/version.h>

extern IRControl irControl;
extern UpTime upTime;

WebServerControl::WebServerControl(int port) : 
      Server(port)
    , Port(port)
{
    Enabled = false;
}

WebServerControl::~WebServerControl() {
    stop();
}

void WebServerControl::begin() {
    if (!Enabled) {
        setupRoutes();
        Server.begin();
        Enabled = true;
        Serial.printf("WebServer started on port %d\n", Port);
    }
}

void WebServerControl::stop() {
    if (Enabled) {
        Server.stop();
        Enabled = false;
        Serial.println("WebServer stopped");
    }
}

void WebServerControl::handleClient() {
    if (Enabled) {
        Server.handleClient();
    }
}

void WebServerControl::setupRoutes() {
    Server.on("/", [this]() { handleRoot(); });
    Server.on("/tx", [this]() { handleTx(); });
    Server.on("/txlog", [this]() { handleTxLog(); });
    Server.on("/rxlog", [this]() { handleRxLog(); });
    Server.onNotFound([this]() { handleNotFound(); });
}

void WebServerControl::handleNotFound() {
    Server.send(404, "text/plain", String("File Not Found\n"));
}

void WebServerControl::handleRoot() {
    String data = getVersionString() + "\n";
    String hostname(Parameter.data.ip.hostname);
    
    data += "Date:          " + getTimeStamp() + "\n";
    data += "Uptime:        " + upTime.toString() + "\n";
    data += "WiFi RSSI:     " + String(WiFi.RSSI()) + "dBm\n";
    data += "\n";
    data += "Tx Data:\n";
    data += "  Count:  " + String(irControl.getTxCount()) + "\n";
    data += "  Last:   " + irControl.getLastTx() + "\n";
    data += "  Log:    http://" + hostname + ".local/txlog\n";
    data += "\n";
    data += "Rx Data:\n";
    data += "  Count:  " + String(irControl.getRxCount()) + "\n";
    data += "  Last:   " + irControl.getLastRx() + "\n";
    data += "  Log:    http://" + hostname + ".local/rxlog\n";
    data += "\n";
    data += "Trigger IR transmission via:\n";
    data += "  http://" + hostname + ".local/tx?type=nec&code=0x1234&repeat=1\n";
    data += "\n";

    Server.send(200, "text/plain", data);
}

void WebServerControl::handleTx() {
    String message;
    decode_type_t type = decode_type_t::NEC;
    uint32_t code = 0;
    uint32_t repeat = 0;
    bool transmit = true;

    for (uint8_t i = 0; i < Server.args(); i++) {
        String tmp = Server.arg(i).c_str();
        const char *arg = tmp.c_str();
        char *endPtr = 0;

        if (Server.argName(i) == "code") {
            uint32_t base = 10;
            if (is32BitHex(arg)) {
                base = 16;    
            }
            code = strtoul(arg, &endPtr, base);
            if (arg == endPtr) {
                message = "ERROR: Invalid code value.\n";
                transmit = false;
                break;
            }
        }
        else if (Server.argName(i) == "type") {
            type = irControl.stringToIRType(arg);
            if (type == decode_type_t::UNKNOWN) {
                message = "ERROR: Unknown type.\n";
                transmit = false;
                break;
            }
        }
        else if (Server.argName(i) == "repeat") {
            repeat = strtoul(arg, &endPtr, 10);
            if (arg == endPtr) {
                message = "ERROR: Invalid repeat value.\n";
                transmit = false;
                break;
            }
            repeat = constrain(repeat, 0, 15);
        }
    }

    if (transmit) {
        irControl.transmit(type, code, repeat);
        message = irControl.getLastTx();
    }  

    Server.send(200, "text/plain", message);
}

void WebServerControl::handleTxLog() {
    String data = irControl.getTxLog();
    Server.send(200, "text/plain", data);   
}

void WebServerControl::handleRxLog() {
    String data = irControl.getRxLog();
    Server.send(200, "text/plain", data);   
}

bool WebServerControl::isEnabled() const {
    return Enabled;
}

int WebServerControl::getPort() const {
    return Port;
}
