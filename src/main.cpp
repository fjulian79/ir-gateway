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
#include <WiFi.h>
#include "time.h"
#include <WebServer.h>
#include <ESPmDNS.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

#include <generic/generic.hpp>
#include <generic/task.hpp>
#include <generic/uptime.hpp>
#include <cli/cli.hpp>
#include <version/version.h>

#include "parameter.hpp"
#include "stringRingBuffer.hpp"

#define IRTX_PIN            22
#define IRRX_PIN            23
#define WIFILED_PIN         2
#define WIFILED_ON          digitalWrite(LED_BUILTIN, 1)
#define WIFILED_OFF         digitalWrite(LED_BUILTIN, 0)

IRsend irSend(IRTX_PIN);
IRrecv irRecv(IRRX_PIN);
decode_results irRxData;
StringRingBuffer lastTx(30);
StringRingBuffer lastRx(30);
uint32_t numTx = 0;
uint32_t numRx = 0;

Task serialTask(10);
Task networkTask(30000);
Cli cli;
UpTime upTime;
MDNSResponder mdns;
WebServer webServer(80);
bool networking_enabled = false;

bool is32BitHex(const char *str, uint8_t maxlen = 10) 
{
    size_t len = strlen(str);
    if (len > maxlen)
    {
        return false;
    }

    if (str[0] != '0' || tolower(str[1]) != 'x')
    {
        return false;
    }

    for (size_t i = 2; i < len; i++)
    {
        if (!isxdigit(str[i])) 
        {
            return false;
        }
    }

    return true;
}

String getTimeStamp(void)
{
    struct tm timeinfo;
    char timeStringBuff[20];

    if (getLocalTime(&timeinfo))
    {
        strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
    }
    else
    {
        sprintf(timeStringBuff, "ntp error");
    }
    return String(timeStringBuff);
}

void transmit_ir(decode_type_t type, uint32_t code, uint16_t repeat = 0)
{
    String hexcode = String(code, HEX);
    String protocol = typeToString(type);
    String ts = getTimeStamp();

    hexcode.toUpperCase();
    lastTx.push(ts + String("; ") + protocol + String("; 0x") + hexcode);
    numTx++;

    irRecv.pause();
    irSend.send(type, code, 32, repeat);
    irRecv.resume();

    Serial.printf("%s IR TX: %s 0x%0X", ts.c_str(), protocol.c_str(), code);
    if(repeat != 0)
    {
        Serial.printf(" (repeat %dx)", repeat);
    }      
    Serial.printf("\n");
}

String getVersionString(void)
{
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

CLI_COMMAND(ver)
{
    Serial.printf("\n%s\n", getVersionString().c_str());
    return 0;
}

CLI_COMMAND(info)
{
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
    Serial.printf("    Count:       %u\n", numTx);
    Serial.printf("    Last:        %s\n", lastTx.peek().c_str());
    Serial.printf("  Rx Data:\n");
    Serial.printf("    Count:       %u\n", numRx);
    Serial.printf("    Last:        %s\n", lastRx.peek().c_str());
    Serial.printf("\n");
    Serial.printf("Network:\n");
    Serial.printf("  WiFi Status:   %s\n", WiFi.isConnected() ? "Connected" : "Connecting ...");
    Serial.printf("  WiFi IP:       %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  WiFi RSSI:     %ddBm\n", WiFi.RSSI());
    Serial.printf("  Homepage:      http://%s.local\n", Parameter.data.ip.hostname);
    Serial.printf("\n");

    return 0;
}

CLI_COMMAND(help)
{
    Serial.printf("Supported commands:\n");
    Serial.printf("  ver                            Prints version infos.\n");
    Serial.printf("  param cmd ...                  Parameter control, supported commands:\n");
    Serial.printf("    clear                        Resets all values to default.\n");
    Serial.printf("    write                        Paste all values at once to the terminal.\n");
    Serial.printf("    set [name]                   Set a single value. use without a name to\n"); 
    Serial.printf("                                 see the list of supported names.\n");
    Serial.printf("    save                         Write the parameter values to the flash.\n");
    Serial.printf("  networking [0/1]               Disables or Enables networking at all.\n");
    Serial.printf("  reset                          Resets the CPU.\n");
    Serial.printf("  tx [type] code [repeat]        Transmits a IR Code \n");
    Serial.printf("                                 type .. optional, ir code, default = NEC\n");
    Serial.printf("                                 code .. the code to send, hex or dec.\n");
    Serial.printf("                                 repeat .. optional, number of Repetitions\n");
    Serial.printf("  help                           Prints this text.\n"); 
    Serial.printf("\n");
    return 0;
}

CLI_COMMAND(reset)
{
    Serial.printf("Resetting the CPU ...\n");
    delay(100);
    ESP.restart();
    return 0;
}

CLI_COMMAND(networking)
{
    networking_enabled = atoi(argv[0]) == 0 ? false : true;
    networkTask.setLastTick(0);
    Serial.printf("Networking %s\n", networking_enabled ? "on" : "off");
    return 0;
}

decode_type_t strtoirtype(const char * const str) 
{
    auto *ptr = reinterpret_cast<const char*>(kAllProtocolNamesStr);
    uint16_t length = strlen(ptr);

    for (uint16_t i = 0; length; i++) 
    {
        if (!strcasecmp(str, ptr))
        {
            return (decode_type_t)i;
        }
        ptr += length + 1;
        length = strlen(ptr);
    }

    return decode_type_t::UNKNOWN;
}

CLI_COMMAND(tx)
{
    decode_type_t type = decode_type_t::NEC;
    uint32_t code = 0;
    uint32_t base  = 10;
    uint16_t repeat = 0;
    uint8_t idx = 0;
    char *endPtr = 0;

    char test[] = "NEC";

    if (argc == 0)
    {
        return -1;
    }

    type = strtoirtype(argv[idx]);
    if(type == decode_type_t::UNKNOWN)
    {
        type = decode_type_t::NEC;
    }
    else
    {
        idx++;
        argc--;    
    }
    
    if(is32BitHex(argv[idx]))
    {
        base = 16;    
    }
    code = strtoul(argv[idx], &endPtr, base);
    if(argv[idx] == endPtr)
    {
        Serial.printf("Invalid arguments\n");
        return -1;
    }
    idx++;

    if (argc > 1)
    {
        repeat = strtoul(argv[idx], NULL, 10);
        repeat = constrain(repeat, 0, 15);
    }
    
    transmit_ir(type, code, repeat);
    
    return 0;
}

CLI_COMMAND(txlog)
{
    Serial.print(lastTx.dump().c_str());
    return 0;
}

CLI_COMMAND(rxlog)
{
    Serial.print(lastRx.dump().c_str());
    return 0;
}

void handleNotFound() 
{
    webServer.send(404, "text/plain", String("File Not Found\n"));
}

void handleRoot()
{
    String data = getVersionString() + "\n";
    String hostname(Parameter.data.ip.hostname);
    
    data += "Date:          " + getTimeStamp() + "\n";
    data += "Uptime:        " + upTime.toString() + "\n";
    data += "WiFi RSSI:     " + String(WiFi.RSSI()) + "dBm\n";
    data += "\n";
    data += "Tx Data:\n";
    data += "  Count:  " + String(numTx) + "\n";
    data += "  Last:   " + lastTx.peek() + "\n";
    data += "  Log:    http://" + hostname + ".local/txlog\n";
    data += "\n";
    data += "Rx Data:\n";
    data += "  Count:  " + String(numRx) + "\n";
    data += "  Last:   " + lastRx.peek() + "\n";
    data += "  Log:    http://" + hostname + ".local/rxlog\n";
    data += "\n";
    data += "Trigger IR transmission via:\n";
    data += "  http://" + hostname + ".local/tx?type=nec&code=0x1234&repeat=1\n";
    data += "\n";

    webServer.send(200, "text/plain", data);
}

void handleTx()
{
    String message;
    decode_type_t type = decode_type_t::NEC;
    uint32_t code = 0;
    uint32_t repeat = 0;
    bool transmit = true;

    for (uint8_t i = 0; i < webServer.args(); i++)
    {
        String tmp = webServer.arg(i).c_str();
        const char *arg = tmp.c_str();
        char *endPtr = 0;

        if (webServer.argName(i) == "code") 
        {
            uint32_t base  = 10;

            if(is32BitHex(arg))
            {
                base = 16;    
            }
            code = strtoul(arg, &endPtr, base);
            if (arg == endPtr)
            {
                message = "ERROR: Invalid code value.\n";
                transmit = false;
                break;
            }
        }
        else if (webServer.argName(i) == "type")
        {
            type = strToDecodeType(arg);
            if(type == decode_type_t::UNKNOWN)
            {
                message = "ERROR: Unknown type.\n";
                transmit = false;
                break;
            }
        }
        else if (webServer.argName(i) == "repeat")
        {
            repeat = strtoul(arg, &endPtr, 10);
            if (arg == endPtr)
            {
                message = "ERROR: Invalid repeat value.\n";
                transmit = false;
                break;
            }
            repeat = constrain(repeat, 0, 15);
        }
    }

    if(transmit)
    {
        transmit_ir(type, code, repeat);
        message = lastTx.peek();
    }  

    webServer.send(200, "text/plain", message);
}

void handleTxLog()
{
    String data = lastTx.dump();
    webServer.send(200, "text/plain", data);   
}

void handleRxLog()
{
    String data = lastRx.dump();
    webServer.send(200, "text/plain", data);   
}

bool setup_wifi(void) 
{
    const uint8_t timeout_sec = 5;
    uint32_t start = 0;
    uint32_t ms = 0;

    WIFILED_ON;

    Serial.printf("WiFi: Connecting to %s, ", Parameter.data.wifi.ssid);
    WiFi.mode(WIFI_STA);

    start = millis(); 

    if (Parameter.data.ip.dhcp == false)
    {
        IPAddress ipaddr, gateway, netmask, dns1, dns2;
        ipaddr.fromString((const char*) Parameter.data.ip.ipaddr);
        gateway.fromString((const char*) Parameter.data.ip.gateway);
        netmask.fromString((const char*) Parameter.data.ip.netmask);
        dns1 = gateway; 
        dns2 = gateway; 

        if (!WiFi.config(ipaddr, gateway, netmask, dns1, dns2)) 
        {
            Serial.println("STA Failed to configure");
            return false;
        }
    }
   
    WiFi.begin(Parameter.data.wifi.ssid, Parameter.data.wifi.pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        ms = millis() - start;
        if(ms > (timeout_sec * 1000))
        {
            Serial.printf("Timeout(%u sec)\n", timeout_sec);
            return false;
        }
    }

    ms = millis() - start;
    Serial.printf("%s (%ums)\n", WiFi.localIP().toString().c_str(), ms);
    randomSeed(micros()); 
    WIFILED_OFF;

    return true;   
}

bool setup_mdns(void)
{
    if (mdns.begin(Parameter.data.ip.hostname)) 
    {
        mdns.addService("http", "tcp", 80);
    }

    return true;
}

bool setup_ntp(void)
{
    configTime(0, 0, Parameter.data.ntp.server);
    setenv("TZ", Parameter.data.ntp.timezone, 1);
    tzset();

    return true;
}

bool setup_webserver(void)
{
    webServer.on("/", handleRoot);
    webServer.on("/tx", handleTx);
    webServer.on("/txlog", handleTxLog);
    webServer.on("/rxlog", handleRxLog);
    webServer.onNotFound(handleNotFound);
    webServer.begin();

    return true;
}

bool setup_ir(void)
{
    pinMode(IRTX_PIN, OUTPUT);
    digitalWrite(IRTX_PIN, false);

    irSend.begin();
    irRecv.enableIRIn();

    return true;
}

/**
 * This task makes only sense on a ESP32 S2 as there the USB CDC Serial
 * is aware of active connections. On all other boards it will work as well,
 * but transition to state 2 and stay there for ever.
 */
void serialTaskFunction(uint32_t now)
{
    static uint8_t state = 0;

    if (Serial && state == 0)
    {
        state = 1;
        return;
    }

    if (state == 1)
    {
        state = 2;
        Serial.println();
        cmd_ver(0, 0);
        cli.reset();
    }

    if (!Serial && state != 0)
    {
        state = 0;
    }

    if(state == 2)
    {
        cli.loop();
    }
}

void setup(void) 
{
    pinMode(WIFILED_PIN, OUTPUT);
    WIFILED_OFF;

    Serial.begin(115200);
    serialTask.setTaskFunction(serialTaskFunction);

    upTime.begin();

    if(Parameter.begin() != true)
    {
        Serial.printf("Error: Invalid parameters.\n");
        param_clear();
    }

    if (Parameter.data.wifi.ssid[0] != 0 && Parameter.data.wifi.pass[0] != 0)
    {
        networking_enabled = true;
        setup_wifi();
    }

    setup_mdns();
    setup_ntp();
    setup_webserver();
    setup_ir();

    cli.begin();
}

void loop(void) 
{
    uint32_t now = millis();

    if (networkTask.isScheduled(now) && networking_enabled)
    {
        if (WiFi.status() != WL_CONNECTED) 
        {
            WIFILED_ON;
            WiFi.disconnect();
            setup_wifi();
        }
    }

    if (irRecv.decode(&irRxData))
    {
        String ts = getTimeStamp();
        String protocol = typeToString(irRxData.decode_type);
        String hexvalue = resultToHexidecimal(&irRxData);
        
        irRecv.resume();

        lastRx.push(ts + String("; ") + protocol + String("; ") + hexvalue);
        numRx++;
        Serial.printf("%s IR RX: %s %s\n", ts.c_str(), protocol.c_str(), hexvalue.c_str());
    }

    webServer.handleClient();
    upTime.loop();
    serialTask.loop(now);
}
