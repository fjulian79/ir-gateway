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
#include <esp_sntp.h>

#include <generic/task.hpp>
#include <generic/uptime.hpp>
#include <cli/cli.hpp>

#include "common.hpp"
#include "parameter.hpp"
#include "ircontrol.hpp"
#include "webservercontrol.hpp"
#include "otacontrol.hpp"
#include "healthcontrol.hpp"
#include "syslog.hpp"

Task networkTask(30000);
Cli cli;
UpTime upTime;
IRControl irControl;
WebServerControl webServerControl;
OtaControl otaControl;
HealthControl healthControl;
bool networking_enabled = false;
bool network_services_started = false;
bool wifi_connected = false;
uint32_t wifi_connect_start = 0;
volatile time_t ntp_last_sync = 0;
bool ntp_first_sync_logged = false;
uint32_t ntp_recovery_last = 0;
uint32_t ntp_recovery_count = 0;
uint8_t ntp_recovery_step = 0;

/**
 * @brief Time after connecting to the WiFi or the last recovery attempt,
 * before NTP is considered as broken without a synchronization.
 */
#define NTP_RECOVERY_TIMEOUT_MS     (15 * 60 * 1000)

/**
 * @brief Maximum age of the last synchronization, sntp synchronizes every 3h.
 */
#define NTP_MAX_SYNC_AGE_S          (7 * 60 * 60)

bool setup_wifi(void);

CLI_COMMAND(ver) {
    ioStream.printf("\n%s\n", getVersionString().c_str());
    return 0;
}

CLI_COMMAND(info) {
    ioStream.printf("ESP32:\n");
    ioStream.printf("  Chip:          %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    ioStream.printf("  CPU's:         %u @ %uMHz\n", ESP.getChipCores(), ESP.getCpuFreqMHz());
    ioStream.printf("  Flash:         %uMB, %uMhz, Mode 0x%0x\n", ESP.getFlashChipSize()/(1024*1024), ESP.getFlashChipSpeed()/1000000, ESP.getFlashChipMode());
    ioStream.printf("  PSRAM:         %u free of %u\n", ESP.getFreePsram(), ESP.getPsramSize());
    ioStream.printf("  Heap:          %u free of %u\n", ESP.getFreeHeap(), ESP.getHeapSize());
    ioStream.printf("  Heap min free: %u\n", ESP.getMinFreeHeap());
    ioStream.printf("  Heap max blk:  %u\n", ESP.getMaxAllocHeap());
    ioStream.printf("  MAC:           %s\n", WiFi.macAddress().c_str());
    ioStream.printf("  Reset reason:  %s%s%s\n", healthControl.getResetReason(),
        healthControl.getRestartReason()[0] ? ", " : "", healthControl.getRestartReason());
    ioStream.printf("  Resets:        %u since power on\n", healthControl.getResetCount());
    ioStream.printf("  Up time:       %s\n", upTime.toString().c_str());
    ioStream.printf("  Date:          %s\n", getTimeStamp().c_str());
    ioStream.printf("\n");
    ioStream.printf("Parameter:\n");
    ioStream.printf("  WiFi:\n");
    ioStream.printf("    SSID:        %s\n", Parameter.data.wifi.ssid);
    ioStream.printf("    Pass:        *****\n");
    ioStream.printf("  Network:\n");
    ioStream.printf("    Hostname:    %s\n", Parameter.data.ip.hostname);
    ioStream.printf("    DHCP:        %s\n", Parameter.data.ip.dhcp ? "true" : "false");
    ioStream.printf("    IP address:  %s\n", Parameter.data.ip.ipaddr);
    ioStream.printf("    Netmask:     %s\n", Parameter.data.ip.netmask);
    ioStream.printf("    Gateway:     %s\n", Parameter.data.ip.gateway);
    ioStream.printf("  NTP:\n");
    ioStream.printf("    Server:      %s\n", Parameter.data.ntp.server);
    ioStream.printf("    Timezone:    %s\n", Parameter.data.ntp.timezone);
    ioStream.printf("\n");
    ioStream.printf("IR:\n");
    ioStream.printf("  Tx Data:\n");
    ioStream.printf("    Count:       %u\n", irControl.getTxCount());
    ioStream.printf("    Last:        %s\n", irControl.getLastTx().c_str());
    ioStream.printf("  Rx Data:\n");
    ioStream.printf("    Count:       %u\n", irControl.getRxCount());
    ioStream.printf("    Last:        %s\n", irControl.getLastRx().c_str());
    ioStream.printf("\n");
    ioStream.printf("Network:\n");
    ioStream.printf("  WiFi Status:   %s\n", WiFi.isConnected() ? "Connected" : "Connecting ...");
    ioStream.printf("  WiFi IP:       %s\n", WiFi.localIP().toString().c_str());
    ioStream.printf("  WiFi RSSI:     %ddBm\n", WiFi.RSSI());
    ioStream.printf("  DNS:           %s\n", WiFi.dnsIP().toString().c_str());
    ioStream.printf("  NTP Sync:      %s, %u recovery attempts\n",
        ntp_last_sync ? getTimeStamp(ntp_last_sync).c_str() : "never", ntp_recovery_count);
    ioStream.printf("  Homepage:      http://%s.local\n", Parameter.data.ip.hostname);
    ioStream.printf("  OTA:           %s, %s, %s\n", otaControl.isEnabled() ? "Ready" : "Disabled",
        OtaControl::isPasswordSet() ? "password protected" : "no password",
        otaControl.isSignatureRequired() ? "signed images only" : "unsigned images accepted");
    ioStream.printf("\n");
    return 0;
}

CLI_COMMAND(help) {
    ioStream.printf("Supported commands:\n");
    ioStream.printf("  ver                            Prints version infos.\n");
    ioStream.printf("  info                           Prints system, parameter, IR and network\n");
    ioStream.printf("                                 status.\n");
    ioStream.printf("  param cmd ...                Parameter control, supported commands:\n");
    ioStream.printf("    clear                        Resets all values to default.\n");
    ioStream.printf("    write                        Paste all values at once to the terminal.\n");
    ioStream.printf("    set [name]                   Set a single value. use without a name to\n"); 
    ioStream.printf("                                 see the list of supported names.\n");
    ioStream.printf("    save                         Write the parameter values to the flash.\n");
    ioStream.printf("  networking [1/0/on/off]        Disables or Enables networking at all.\n");
    ioStream.printf("  reset                          Resets the CPU.\n");
    ioStream.printf("  tx [type] code [repeat]        Transmits a IR Code \n");
    ioStream.printf("                                 type .. optional, ir code, default = NEC\n");
    ioStream.printf("                                 code .. the code to send, hex or dec.\n");
    ioStream.printf("                                 repeat .. optional, number of Repetitions\n");
    ioStream.printf("  txlog                          Prints the log of transmitted IR codes.\n");
    ioStream.printf("  rxlog                          Prints the log of received IR codes.\n");
    ioStream.printf("  syslog                         Prints the system log.\n");
    ioStream.printf("  help                          Prints this text.\n"); 
    ioStream.printf("\n");
    return 0;
}

CLI_COMMAND(reset) {
    healthControl.restart("CLI reset command");
    return 0;
}

CLI_COMMAND(networking) {
    bool enable = false;

    if (argc != 1) {
        ioStream.printf("Error: Wrong number of arguments. Usage: networking [1/0/on/off]\n");
        return -1;
    }

    if (strcmp(argv[0], "1") == 0 || strcmp(argv[0], "on") == 0) {
        enable = true;
    } else if (strcmp(argv[0], "0") == 0 || strcmp(argv[0], "off") == 0) {
        enable = false;
    } else {
        ioStream.printf("Error: Invalid argument. Usage: networking [1/0/on/off]\n");
        return -1;
    }   

    if (enable && Parameter.data.wifi.ssid[0] == 0) {
        ioStream.printf("Error: No WiFi configured, see param set ssid\n");
        return -1;
    }

    if (enable == networking_enabled) {
        ioStream.printf("Networking already %s\n", networking_enabled ? "on" : "off");
        return 0;
    }

    networking_enabled = enable;
    ioStream.printf("Networking %s\n", networking_enabled ? "on" : "off");
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
    ioStream.print(irControl.getTxLog().c_str());
    return 0;
}

CLI_COMMAND(rxlog) {
    ioStream.print(irControl.getRxLog().c_str());
    return 0;
}

CLI_COMMAND(syslog) {
    sysLog.printTo(ioStream);
    return 0;
}

bool setup_mdns(void) {
    if (!MDNS.begin(Parameter.data.ip.hostname)) {
        sysLog.printf("mDNS: Failed to start\n");
        return false;
    }

    if (!MDNS.addService("http", "tcp", webServerControl.getPort())) {
        sysLog.printf("mDNS: Failed to add http service\n");
        return false;
    }

    sysLog.printf("mDNS: Started as %s.local\n", Parameter.data.ip.hostname);
    return true;
}

bool setup_ntp(void) {
    /* sntp keeps the pointers, so only pass strings with static lifetime */
    static const char fallback_server[] = "pool.ntp.org";
    const char *server2 = strcmp(Parameter.data.ntp.server, fallback_server) ? fallback_server : nullptr;

    /* Runs in the tcpip task, so it must not log, see handle_ntp() */
    sntp_set_time_sync_notification_cb([](struct timeval *tv) {
        ntp_last_sync = tv->tv_sec;
    });
    configTzTime(Parameter.data.ntp.timezone, Parameter.data.ntp.server, server2);
    sysLog.printf("NTP: Started, server %s%s%s\n", Parameter.data.ntp.server,
        server2 ? ", fallback " : "", server2 ? server2 : "");

    return true;
}

/**
 * @brief Recovers NTP if it did not synchronize for too long, has to be called
 * from loop(). Alternately restarts the NTP client, which resolves the server
 * name again, and reconnects the WiFi to get fresh DHCP data, e.g. the DNS
 * server. E.g. ASUS routers resolve every name to their own address as long
 * as the internet connection is down after a power outage.
 */
void handle_ntp(uint32_t now) {
    bool synced = ntp_last_sync != 0 && (time(nullptr) - ntp_last_sync) < NTP_MAX_SYNC_AGE_S;

    if (ntp_last_sync != 0 && !ntp_first_sync_logged) {
        sysLog.printf("NTP: Time synchronized\n");
        ntp_first_sync_logged = true;
    }

    if (!wifi_connected || !network_services_started) {
        return;
    }

    if (synced) {
        if (ntp_recovery_step != 0) {
            sysLog.printf("NTP: Recovered after %u attempts\n", ntp_recovery_step);
            ntp_recovery_step = 0;
        }
        ntp_recovery_last = now;
        return;
    }

    if (now - ntp_recovery_last < NTP_RECOVERY_TIMEOUT_MS) {
        return;
    }

    ntp_recovery_last = now;
    ntp_recovery_count++;
    if (ntp_recovery_step % 2 == 0) {
        sysLog.printf("NTP: No synchronization, restarting NTP client\n");
        setup_ntp();
    } else {
        sysLog.printf("NTP: Still no synchronization, reconnecting WiFi\n");
        WiFi.reconnect();
    }
    ntp_recovery_step++;
}

/**
 * @brief Starts connecting to the WiFi, does not wait for the connection.
 * Reconnecting is handled by the WiFi stack, see handle_wifi() for the
 * connection state handling.
 */
bool setup_wifi(void) {
    WIFILED_OFF;
    sysLog.printf("WiFi: Connecting to %s\n", Parameter.data.wifi.ssid);
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
            sysLog.printf("WiFi: Failed to configure the static IP\n");
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
            sysLog.printf("WiFi: Still not connected, retrying\n");
            WiFi.reconnect();
        }
        return;
    }

    wifi_connected = connected;
    if (!connected) {
        WIFILED_OFF;
        sysLog.printf("WiFi: Connection lost\n");
        networkTask.setLastTick(now);
        wifi_connect_start = now;
        return;
    }

    WIFILED_ON;
    sysLog.printf("WiFi: Connected, %s (%ums)\n", WiFi.localIP().toString().c_str(),
        now - wifi_connect_start);
        
    /* Reset the NTP recovery timer on successful connection */
    ntp_recovery_last = now;

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
        sysLog.printf("Parameter: Invalid, using defaults\n");
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
    healthControl.begin();
}

void loop(void) {
    uint32_t now = millis();

    handle_wifi(now);
    handle_ntp(now);
    irControl.handleReceive();
    webServerControl.handleClient();
    otaControl.handle();
    healthControl.loop(now, networking_enabled);
    upTime.loop();
    cli.loop();
}
