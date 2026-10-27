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

#include "healthcontrol.hpp"
#include <WiFi.h>
#include <esp_task_wdt.h>
#include "common.hpp"

/**
 * @brief Interval of the health checks in ms.
 */
#define HEALTH_CHECK_INTERVAL_MS        1000

/**
 * @brief Marks the data in RTC memory as valid, it is random after power on.
 */
#define HEALTH_RTC_MAGIC                0x4952474Bu

/**
 * @brief Data surviving a reset, but not a power cycle.
 */
typedef struct {
    uint32_t magic;
    uint32_t resetCount;
    char reason[64];
} HealthRtcData_t;

RTC_NOINIT_ATTR static HealthRtcData_t rtcData;

/**
 * @brief Reason of the restart triggered by software, copied from rtcData
 * on boot.
 */
static char restartReason[sizeof(rtcData.reason)] = "";

HealthControl::HealthControl() :
      WiFiLastSeen(0)
    , LastCheck(0)
{

}

void HealthControl::begin() {
    esp_reset_reason_t reset = esp_reset_reason();

    if (rtcData.magic != HEALTH_RTC_MAGIC || reset == ESP_RST_POWERON || reset == ESP_RST_BROWNOUT) {
        memset(&rtcData, 0, sizeof(rtcData));
        rtcData.magic = HEALTH_RTC_MAGIC;
    } else {
        rtcData.resetCount++;
    }

    if (reset == ESP_RST_SW) {
        rtcData.reason[sizeof(rtcData.reason) - 1] = 0;
        strcpy(restartReason, rtcData.reason);
    }
    rtcData.reason[0] = 0;

    sysLog.notice("Health: Reset reason %s%s%s, %lu resets since power on",
        getResetReason(), restartReason[0] ? ", " : "", restartReason, (unsigned long) rtcData.resetCount);

    /* Keep supervising the idle task of core 0 as configured by the core */
    esp_task_wdt_config_t config = {
        .timeout_ms = HEALTH_WDT_TIMEOUT_S * 1000,
        .idle_core_mask = (1 << 0),
        .trigger_panic = true,
    };
    if (esp_task_wdt_reconfigure(&config) != ESP_OK) {
        sysLog.err("Health: Failed to configure the watchdog");
    }
    enableLoopWDT();

    WiFiLastSeen = millis();
}

void HealthControl::loop(uint32_t now, bool wifiExpected) {
    if (now - LastCheck < HEALTH_CHECK_INTERVAL_MS) {
        return;
    }

    LastCheck = now;
    checkWiFi(now, wifiExpected);
    checkHeap();
}

void HealthControl::checkWiFi(uint32_t now, bool wifiExpected) {
    if (!wifiExpected || WiFi.isConnected()) {
        WiFiLastSeen = now;
        return;
    }

    if (now - WiFiLastSeen > HEALTH_WIFI_LOST_TIMEOUT_MS) {
        restart("WiFi lost for 10 min");
    }
}

void HealthControl::checkHeap() {
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    char reason[sizeof(rtcData.reason)];

    if (maxBlock < HEALTH_MIN_HEAP_BLOCK) {
        snprintf(reason, sizeof(reason), "Low memory, largest block %u bytes", maxBlock);
        restart(reason);
    }
}

void HealthControl::restart(const char *reason) {
    strncpy(rtcData.reason, reason, sizeof(rtcData.reason) - 1);
    rtcData.reason[sizeof(rtcData.reason) - 1] = 0;

    sysLog.err("Health: Restarting, %s", rtcData.reason);
    Serial.flush();
    delay(100);
    ESP.restart();
}

const char *HealthControl::getResetReason() const {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "Power on";
        case ESP_RST_EXT:       return "External pin";
        case ESP_RST_SW:        return "Software";
        case ESP_RST_PANIC:     return "Panic";
        case ESP_RST_INT_WDT:   return "Interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "Task watchdog";
        case ESP_RST_WDT:       return "Other watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep sleep";
        case ESP_RST_BROWNOUT:  return "Brownout";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "Unknown";
    }
}

const char *HealthControl::getRestartReason() const {
    return restartReason;
}

uint32_t HealthControl::getResetCount() const {
    return rtcData.resetCount;
}
