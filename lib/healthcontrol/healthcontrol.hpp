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

#pragma once

#include <Arduino.h>

/**
 * @brief Timeout of the loop watchdog in seconds.
 * Long running operations like OTA updates, IR sequences or waiting for
 * serial input have to call feedLoopWDT() to not trigger it.
 */
#define HEALTH_WDT_TIMEOUT_S            30

/**
 * @brief Time in ms the WiFi may be disconnected before the device restarts.
 */
#define HEALTH_WIFI_LOST_TIMEOUT_MS     (10 * 60 * 1000)

/**
 * @brief Minimum size of the largest free heap block in bytes, the device
 * restarts if it falls below.
 */
#define HEALTH_MIN_HEAP_BLOCK           (8 * 1024)

/**
 * @brief Health control class.
 * This class supervises the device by a loop watchdog and periodic health
 * checks, restarts the device if needed and keeps track of the reason.
 */
class HealthControl {

    public:

        /**
         * @brief Constructor for HealthControl.
         */
        HealthControl();

        /**
         * @brief Enable the loop watchdog and evaluate the last reset.
         * Has to be called at the end of setup().
         */
        void begin();

        /**
         * @brief Run the periodic health checks.
         * Has to be called periodically from the main loop.
         * @param now The current time in ms.
         * @param wifiExpected true if the WiFi shall be connected.
         */
        void loop(uint32_t now, bool wifiExpected);

        /**
         * @brief Restart the device and remember the reason.
         * @param reason The reason of the restart, shown after the reboot.
         */
        void restart(const char *reason);

        /**
         * @brief Get the hardware reset reason as text.
         * @return The reset reason of the current boot.
         */
        const char *getResetReason() const;

        /**
         * @brief Get the reason of the last restart triggered by software.
         * @return The reason passed to restart(), or an empty string if the
         *         last reset was not triggered by restart().
         */
        const char *getRestartReason() const;

        /**
         * @brief Get the number of resets since power on.
         * @return The number of resets, 0 after power on.
         */
        uint32_t getResetCount() const;

    private:

        /**
         * @brief Check the WiFi connection.
         */
        void checkWiFi(uint32_t now, bool wifiExpected);

        /**
         * @brief Check the heap.
         */
        void checkHeap();

        /**
         * @brief Time in ms when the WiFi was seen connected the last time.
         */
        uint32_t WiFiLastSeen;

        /**
         * @brief Time in ms of the last health check.
         */
        uint32_t LastCheck;

};
