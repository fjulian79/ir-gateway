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

#include "webservercontrol.hpp"
#include <WiFi.h>
#include "parameter.hpp"
#include "ircontrol.hpp"
#include "otacontrol.hpp"
#include "healthcontrol.hpp"
#include "syslog.hpp"
#include <generic/uptime.hpp>
#include <version/version.h>

extern IRControl irControl;
extern UpTime upTime;
extern OtaControl otaControl;
extern HealthControl healthControl;
extern volatile time_t ntp_last_sync;
extern uint32_t ntp_recovery_count;

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
        sysLog.info("WebServer: Started on port %d", Port);
    }
}

void WebServerControl::stop() {
    if (Enabled) {
        Server.stop();
        Enabled = false;
        sysLog.info("WebServer: Stopped");
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
    Server.on("/txseq", [this]() { handleTxSequence(); });
    Server.on("/txlog", [this]() { handleTxLog(); });
    Server.on("/rxlog", [this]() { handleRxLog(); });
    Server.on("/syslog", [this]() { handleSysLog(); });
    /* POST only, a GET could be triggered by any web page, e.g. via <img> */
    Server.on("/reset", HTTP_POST, [this]() { handleReset(); });
    Server.on("/reset", HTTP_GET, [this]() {
        Server.send(405, "text/plain", "Use POST, e.g. curl -X POST http://"
            + String(Parameter.data.ip.hostname) + ".local/reset\n");
    });
    Server.onNotFound([this]() { handleNotFound(); });
}

void WebServerControl::handleNotFound() {
    Server.send(404, "text/plain", String("File Not Found\n"));
}

void WebServerControl::handleRoot() {
    String data;
    String hostname(Parameter.data.ip.hostname);

    data.reserve(2048);
    data += getVersionString() + "\n";
    data += "System:\n";
    data += "  Date:         " + getTimeStamp() + "\n";
    data += "  Uptime:       " + upTime.toString() + "\n";
    data += "  Reset reason: " + String(healthControl.getResetReason())
        + (healthControl.getRestartReason()[0] ? ", " : "") + healthControl.getRestartReason() + "\n";
    data += "  Resets:       " + String(healthControl.getResetCount()) + " since power on\n";
    data += "  Heap:         " + String(ESP.getFreeHeap()) + " free, "
        + String(ESP.getMinFreeHeap()) + " min free, "
        + String(ESP.getMaxAllocHeap()) + " max block\n";
    data += "  Log:          http://" + hostname + ".local/syslog\n";
    data += "\n";
    data += "Network:\n";
    data += "  IP:           " + WiFi.localIP().toString() + "\n";
    data += "  DNS:          " + WiFi.dnsIP().toString() + "\n";
    data += "  WiFi RSSI:    " + String(WiFi.RSSI()) + "dBm\n";
    data += "  NTP sync:     " + (ntp_last_sync ? getTimeStamp(ntp_last_sync) : String("never"))
        + ", " + String(ntp_recovery_count) + " recovery attempts\n";
    data += "  OTA:          " + String(otaControl.isEnabled() ? "ready" : "disabled")
        + (OtaControl::isPasswordSet() ? ", password protected" : ", no password")
        + (otaControl.isSignatureRequired() ? ", signed images only" : ", unsigned images accepted") + "\n";
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

void WebServerControl::handleTxSequence() {
    String message;
    String sequence;

    for (uint8_t i = 0; i < Server.args(); i++) {
        if (Server.argName(i) == "sequence") {
            sequence = Server.arg(i);
            break;
        }
    }

    if (sequence.length() == 0) {
        message = "ERROR: Missing sequence parameter.\n";
        message += "Format: /txseq?sequence=type:code:repeat:pause,type:code:repeat:pause,...\n";
        message += "Example: /txseq?sequence=nec:0x1234:1:500,nec:0x5678:2:1000\n";
        message += "Pause is in milliseconds (optional, default=100ms)\n";
        Server.send(400, "text/plain", message);
        return;
    }

    int executed = executeSequence(sequence, message);
    
    if (executed >= 0) {
        message = "Sequence executed: " + String(executed) + " commands\n" + message;
        Server.send(200, "text/plain", message);
    } else {
        Server.send(400, "text/plain", message);
    }
}

int WebServerControl::executeSequence(const String& sequence, String& errorMessage) {
    String command;
    int commandCount = 0;
    int startPos = 0;
    int commaPos = 0;

    do {
        commaPos = sequence.indexOf(',', startPos);
        if (commaPos == -1) {
            command = sequence.substring(startPos);
        } else {
            command = sequence.substring(startPos, commaPos);
        }

        command.trim();
        if (command.length() == 0) {
            break;
        }

        if (!executeSequenceCommand(command, errorMessage)) {
            return -1;
        }

        commandCount++;
        startPos = commaPos + 1;
    } while (commaPos != -1);

    return commandCount;
}

bool WebServerControl::executeSequenceCommand(const String& command, String& errorMessage) {
    int colonPos1 = command.indexOf(':');
    int colonPos2 = command.indexOf(':', colonPos1 + 1);
    int colonPos3 = command.indexOf(':', colonPos2 + 1);

    if (colonPos1 == -1 || colonPos2 == -1) {
        errorMessage = "ERROR: Invalid command format: " + command + "\n";
        errorMessage += "Expected: type:code:repeat[:pause]\n";
        return false;
    }

    String typeStr = command.substring(0, colonPos1);
    String codeStr = command.substring(colonPos1 + 1, colonPos2);
    String repeatStr = command.substring(colonPos2 + 1, colonPos3 == -1 ? command.length() : colonPos3);
    String pauseStr = colonPos3 == -1 ? "100" : command.substring(colonPos3 + 1);

    decode_type_t type = irControl.stringToIRType(typeStr.c_str());
    if (type == decode_type_t::UNKNOWN) {
        errorMessage = "ERROR: Unknown type: " + typeStr + "\n";
        return false;
    }

    char* endPtr;
    uint32_t base = is32BitHex(codeStr.c_str()) ? 16 : 10;
    uint32_t code = strtoul(codeStr.c_str(), &endPtr, base);
    if (codeStr.c_str() == endPtr) {
        errorMessage = "ERROR: Invalid code: " + codeStr + "\n";
        return false;
    }

    uint32_t repeat = strtoul(repeatStr.c_str(), &endPtr, 10);
    if (repeatStr.c_str() == endPtr) {
        errorMessage = "ERROR: Invalid repeat: " + repeatStr + "\n";
        return false;
    }
    repeat = constrain(repeat, 0, 15);

    uint32_t pause = strtoul(pauseStr.c_str(), &endPtr, 10);
    if (pauseStr.c_str() == endPtr) {
        errorMessage = "ERROR: Invalid pause: " + pauseStr + "\n";
        return false;
    }
    pause = constrain(pause, 0, 5000);

    irControl.transmit(type, code, repeat);

    if (pause > 0) {
        delay(pause);
    }

    /* Sequences may take longer than the loop watchdog timeout */
    feedLoopWDT();
    return true;
}

void WebServerControl::handleReset() {
    Server.send(200, "text/plain", "Restarting ...\n");
    healthControl.restart("Web reset request");
}

void WebServerControl::handleTxLog() {
    sendEventLog(irControl.getTxLog());
}

void WebServerControl::handleRxLog() {
    sendEventLog(irControl.getRxLog());
}

void WebServerControl::sendEventLog(const EventLog &log) {
    /* Written directly to the client like the system log */
    Server.setContentLength(log.getLength());
    Server.send(200, "text/plain", "");
    log.printTo(Server.client());
}

void WebServerControl::handleSysLog() {
    /* Written directly to the client, a String would need a heap block of
     * the size of the whole log */
    Server.setContentLength(sysLog.getLength());
    Server.send(200, "text/plain", "");
    sysLog.printTo(Server.client());
}

bool WebServerControl::isEnabled() const {
    return Enabled;
}

int WebServerControl::getPort() const {
    return Port;
}
