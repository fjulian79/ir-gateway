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
 * @brief Password protecting the OTA updates.
 * Set at build time by scripts/ota_pre.py from secrets/ota_password. An empty
 * password disables the authentication.
 */
#ifndef OTA_PASSWORD
#define OTA_PASSWORD        ""
#endif

/**
 * @brief OTA control class.
 * This class handles firmware updates via the ArduinoOTA protocol, which is
 * used by PlatformIO (upload_protocol = espota). Only firmware images signed
 * with the private key in secrets/ are accepted, see scripts/ota_post.py.
 */
class OtaControl {

    public:

        /**
         * @brief Constructor for OtaControl.
         */
        OtaControl();

        /**
         * @brief Start listening for OTA updates.
         * Has to be called after the network is up, further calls are ignored.
         * @param hostname The hostname reported to the OTA client.
         */
        void begin(const char *hostname);

        /**
         * @brief Handle incoming OTA requests.
         * Has to be called periodically from the main loop.
         */
        void handle();

        /**
         * @brief Check if OTA updates are enabled.
         * @return true if begin() has been called, false otherwise.
         */
        bool isEnabled() const;

        /**
         * @brief Check if the OTA updates are password protected.
         * @return true if a password has been set at build time.
         */
        static bool isPasswordSet();

        /**
         * @brief Check if the OTA updates require a signed firmware image.
         * @return true if signature verification is active.
         */
        bool isSignatureRequired() const;

    private:

        /**
         * @brief Flag indicating if OTA updates are enabled.
         */
        bool Enabled;

        /**
         * @brief Flag indicating if signature verification is active.
         */
        bool SignatureRequired;

};
