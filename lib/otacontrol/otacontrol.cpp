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

#include "otacontrol.hpp"
#include <ArduinoOTA.h>

#ifdef UPDATE_SIGN
#include <Updater_Signing.h>
#include <ota_public_key.h>
#endif

OtaControl::OtaControl() {
    Enabled = false;
#ifdef UPDATE_SIGN
    SignatureRequired = true;
#else
    SignatureRequired = false;
#endif
}

void OtaControl::begin(const char *hostname) {
    if (Enabled) {
        return;
    }

    ArduinoOTA.setHostname(hostname);
    /* mDNS is already handled in main, the OTA client is addressed by name */
    ArduinoOTA.setMdnsEnabled(false);
    if (isPasswordSet()) {
        ArduinoOTA.setPassword(OTA_PASSWORD);
    }

#ifdef UPDATE_SIGN
    static UpdaterECDSAVerifier verifier(PUBLIC_KEY, PUBLIC_KEY_LEN, HASH_SHA256);
    ArduinoOTA.setSignature(&verifier);
#endif

    ArduinoOTA.onStart([]() {
        Serial.printf("OTA: Update started\n");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static uint8_t last = 0;
        uint8_t percent = (progress * 100) / total;
        if (percent / 10 != last / 10) {
            Serial.printf("OTA: %u%%\n", percent);
        }
        last = percent;
    });
    ArduinoOTA.onEnd([]() {
        Serial.printf("OTA: Update done, rebooting ...\n");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA: Error %u\n", error);
    });

    ArduinoOTA.begin();
    Enabled = true;
    Serial.printf("OTA: Ready, %s, %s\n",
        isPasswordSet() ? "password protected" : "no password",
        SignatureRequired ? "signed images only" : "unsigned images accepted");
}

void OtaControl::handle() {
    if (Enabled) {
        ArduinoOTA.handle();
    }
}

bool OtaControl::isEnabled() const {
    return Enabled;
}

bool OtaControl::isPasswordSet() {
    return OTA_PASSWORD[0] != 0;
}

bool OtaControl::isSignatureRequired() const {
    return SignatureRequired;
}
