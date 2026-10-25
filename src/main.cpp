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
#include <WiFi.h>
#include "time.h"
#include <ESPmDNS.h>

#include <generic/task.hpp>
#include <generic/uptime.hpp>
#include <cli/cli.hpp>

#include "common.hpp"
#include "parameter.hpp"
#include "ircontrol.hpp"
#include "webservercontrol.hpp"
#include "otacontrol.hpp"

Task networkTask(30000);
Cli cli;
UpTime upTime;
MDNSResponder mdns;
IRControl irControl;
WebServerControl webServerControl;
OtaControl otaControl;
bool networking_enabled = false;
bool network_services_started = false;
bool wifi_connected = false;
uint32_t wifi_connect_start = 0;

bool setup_wifi(void);

CLI_COMMAND(ver) {
    Serial.printf("\n%s\n", getVersionString().c_str());
    return 0;
}

CLI_COMMAND(info) {
    Serial.printf("ESP32:\n");
    Serial.printf("  Chip:          %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("  CPU's:         %u @ %uMHz\n", ESP.getChipCores(), ESP.getCpuFreqMHz());
    Serial.printf("  Flash:         %uMB, %uMhz, Mode 0x%0x\n", ESP.getFlashChipSize()/(1024*1024), ESP.getFlashChipSpeed()/1000000, ESP.getFlashChipMode());
    Serial.printf("  PSRAM:         %u free of %u\n", ESP.getFreePsram(), ESP.getPsramSize());
    Serial.printf("  Heap:          %u free of %u\n", ESP.getFreeHeap(), ESP.getHeapSize());
    Serial.printf("  MAC:           %s\n", WiFi.macAddress().c_str());
    Serial.printf("  Reset reason:  %u\n", esp_reset_reason());
    Serial.printf("  Up time:       %s\n", upTime.toString().c_str());
    Serial.printf("  Date:          %s\n", getTimeStamp().c_str());
    Serial.printf("\n");
    Serial.printf("Parameter:\n");
    Serial.printf("  WiFi:\n");
    Serial.printf("    SSID:        %s\n", Parameter.data.wifi.ssid);
    Serial.printf("    Pass:        *****\n");
    Serial.printf("  Network:\n");
    Serial.printf("    Hostname:    %s\n", Parameter.data.ip.hostname);
    Serial.printf("    DHCP:        %s\n", Parameter.data.ip.dhcp ? "true" : "false");
    Serial.printf("    IP-Adress    %s\n", Parameter.data.ip.ipaddr);
    Serial.printf("    Netmask:     %s\n", Parameter.data.ip.netmask);
    Serial.printf("    Gateway      %s\n", Parameter.data.ip.gateway);
    Serial.printf("  NTP:\n");
    Serial.printf("    Server:      %s\n", Parameter.data.ntp.server);
    Serial.printf("    Timezone:    %s\n", Parameter.data.ntp.timezone);
    Serial.printf("\n");
    Serial.printf("IR:\n");
    Serial.printf("  Tx Data:\n");
    Serial.printf("    Count:       %u\n", irControl.getTxCount());
    Serial.printf("    Last:        %s\n", irControl.getLastTx().c_str());
    Serial.printf("  Rx Data:\n");
    Serial.printf("    Count:       %u\n", irControl.getRxCount());
    Serial.printf("    Last:        %s\n", irControl.getLastRx().c_str());
    Serial.printf("\n");
    Serial.printf("Network:\n");
    Serial.printf("  WiFi Status:   %s\n", WiFi.isConnected() ? "Connected" : "Connecting ...");
    Serial.printf("  WiFi IP:       %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  WiFi RSSI:     %ddBm\n", WiFi.RSSI());
    Serial.printf("  Homepage:      http://%s.local\n", Parameter.data.ip.hostname);
    Serial.printf("  OTA:           %s, %s, %s\n", otaControl.isEnabled() ? "Ready" : "Disabled",
        OtaControl::isPasswordSet() ? "password protected" : "no password",
        otaControl.isSignatureRequired() ? "signed images only" : "unsigned images accepted");
    Serial.printf("\n");
    return 0;
}

CLI_COMMAND(help) {
    Serial.printf("Supported commands:\n");
    Serial.printf("  ver                            Prints version infos.\n");
    Serial.printf("  param cmd ...                  Parameter control, supported commands:\n");
    Serial.printf("    clear                        Resets all values to default.\n");
    Serial.printf("    write                        Paste all values at once to the terminal.\n");
    Serial.printf("    set [name]                   Set a single value. use without a name to\n"); 
    Serial.printf("                                 see the list of supported names.\n");
    Serial.printf("    save                         Write the parameter values to the flash.\n");
    Serial.printf("  networking [1/0/on/off]        Disables or Enables networking at all.\n");
    Serial.printf("  reset                          Resets the CPU.\n");
    Serial.printf("  tx [type] code [repeat]        Transmits a IR Code \n");
    Serial.printf("                                 type .. optional, ir code, default = NEC\n");
    Serial.printf("                                 code .. the code to send, hex or dec.\n");
    Serial.printf("                                 repeat .. optional, number of Repetitions\n");
    Serial.printf("  help                           Prints this text.\n"); 
    Serial.printf("\n");
    return 0;
}

CLI_COMMAND(reset) {
    Serial.printf("Resetting the CPU ...\n");
    delay(100);
    ESP.restart();
    return 0;
}

CLI_COMMAND(networking) {
    bool enable = false;

    if (argc != 1) {
        Serial.printf("Error: Wrong number of arguments. Usage: networking [1/0/on/off]\n");
        return -1;
    }

    if (strcmp(argv[0], "1") == 0 || strcmp(argv[0], "on") == 0) {
        enable = true;
    } else if (strcmp(argv[0], "0") == 0 || strcmp(argv[0], "off") == 0) {
        enable = false;
    } else {
        Serial.printf("Error: Invalid argument. Usage: networking [1/0/on/off]\n");
        return -1;
    }   

    if (enable && Parameter.data.wifi.ssid[0] == 0) {
        Serial.printf("Error: No WiFi configured, see param set ssid\n");
        return -1;
    }

    if (enable == networking_enabled) {
        Serial.printf("Networking already %s\n", networking_enabled ? "on" : "off");
        return 0;
    }

    networking_enabled = enable;
    Serial.printf("Networking %s\n", networking_enabled ? "on" : "off");
    if (networking_enabled) {
        setup_wifi();
    } else {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
    return 0;
}

CLI_COMMAND(tx) {
    if (argc == 1) {
        return irControl.transmit("nec", argv[0], "1");
    } else if (argc == 2) {
        return irControl.transmit(argv[0], argv[1], "1");
    } else if (argc == 3) {
        return irControl.transmit(argv[0], argv[1], argv[2]);
    } else {
        return -1;
    }
}

CLI_COMMAND(txlog) {
    Serial.print(irControl.getTxLog().c_str());
    return 0;
}

CLI_COMMAND(rxlog) {
    Serial.print(irControl.getRxLog().c_str());
    return 0;
}

bool setup_mdns(void) {
    if (mdns.begin(Parameter.data.ip.hostname)) {
        mdns.addService("http", "tcp", 80);
    }

    return true;
}

bool setup_ntp(void) {
    configTime(0, 0, Parameter.data.ntp.server);
    setenv("TZ", Parameter.data.ntp.timezone, 1);
    tzset();

    return true;
}

/**
 * @brief Starts connecting to the WiFi, does not wait for the connection.
 * Reconnecting is handled by the WiFi stack, see handle_wifi() for the
 * connection state handling.
 */
bool setup_wifi(void) {
    WIFILED_OFF;
    Serial.printf("WiFi: Connecting to %s\n", Parameter.data.wifi.ssid);
    WiFi.setHostname(Parameter.data.ip.hostname);
    WiFi.mode(WIFI_STA);
    /* Power save delays incoming requests by up to the beacon interval */
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);

    if (Parameter.data.ip.dhcp == false) {
        IPAddress ipaddr, gateway, netmask, dns1, dns2;
        ipaddr.fromString((const char*) Parameter.data.ip.ipaddr);
        gateway.fromString((const char*) Parameter.data.ip.gateway);
        netmask.fromString((const char*) Parameter.data.ip.netmask);
        dns1 = gateway; 
        dns2 = gateway; 

        if (!WiFi.config(ipaddr, gateway, netmask, dns1, dns2)) {
            Serial.println("STA Failed to configure");
            return false;
        }
    }
   
    WiFi.begin(Parameter.data.wifi.ssid, Parameter.data.wifi.pass);
    wifi_connect_start = millis();

    return true;
}

/**
 * @brief Tracks the WiFi connection state, has to be called from loop().
 * The network services are started once on the first connect, they keep
 * running across reconnects.
 */
void handle_wifi(uint32_t now) {
    bool connected = WiFi.isConnected();

    if (connected == wifi_connected) {
        /* Fallback in case the WiFi stack gave up reconnecting */
        if (!connected && networking_enabled && networkTask.isScheduled(now)) {
            Serial.printf("WiFi: Still not connected, retrying\n");
            WiFi.reconnect();
        }
        return;
    }

    wifi_connected = connected;
    if (!connected) {
        WIFILED_OFF;
        Serial.printf("WiFi: Connection lost\n");
        networkTask.setLastTick(now);
        wifi_connect_start = now;
        return;
    }

    WIFILED_ON;
    Serial.printf("WiFi: Connected, %s (%ums)\n", WiFi.localIP().toString().c_str(),
        now - wifi_connect_start);

    if (!network_services_started) {
        setup_mdns();
        setup_ntp();
        otaControl.begin(Parameter.data.ip.hostname);
        randomSeed(micros());
        network_services_started = true;
    }
}

void setup(void) {
    pinMode(DEBUG_A_PIN, OUTPUT);
    digitalWrite(DEBUG_A_PIN, LOW);
    pinMode(DEBUG_B_PIN, OUTPUT);
    digitalWrite(DEBUG_B_PIN, LOW);
    pinMode(WIFILED_PIN, OUTPUT);
    WIFILED_OFF;

    Serial.begin(115200);
    Serial.println();
    cmd_ver(Serial, 0, 0);

    if(Parameter.begin() != true) {
        Serial.printf("Error: Invalid parameters.\n");
        param_clear();
    }

    if (Parameter.data.wifi.ssid[0] != 0 && Parameter.data.wifi.pass[0] != 0) {
        networking_enabled = true;
        setup_wifi();
    }

    webServerControl.begin();
    upTime.begin();
    irControl.begin();
    cli.begin();
}

void loop(void) {
    uint32_t now = millis();

    handle_wifi(now);
    irControl.handleReceive();
    webServerControl.handleClient();
    otaControl.handle();
    upTime.loop();
    cli.loop();
}
