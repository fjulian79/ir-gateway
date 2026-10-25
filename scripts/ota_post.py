# ir-gateway, build to automate ir remote control commands in smart homes.
#
# Copyright (C) 2026 Julian Friedrich
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# You can file issues at https://github.com/fjulian79/ir-gateway/issues
#
# OTA post script: Signs every firmware image and makes the espota upload use
# the signed image and the OTA password. See ota_pre.py for the secrets.

import os
import subprocess

Import("env")

SECRETS_DIR = os.path.join(env.subst("$PROJECT_DIR"), "secrets")
PASSWORD_FILE = os.path.join(SECRETS_DIR, "ota_password")
PRIVATE_KEY_FILE = os.path.join(SECRETS_DIR, "ota_signing_key.pem")
FIRMWARE_BIN = "$BUILD_DIR/${PROGNAME}.bin"
SIGNED_BIN = "$BUILD_DIR/${PROGNAME}_signed.bin"


def sign_firmware(source, target, env):
    tool = os.path.join(
        env.PioPlatform().get_package_dir("framework-arduinoespressif32"),
        "tools", "bin_signing.py")
    subprocess.run([env.subst("$PYTHONEXE"), tool, "--bin", env.subst(FIRMWARE_BIN),
                    "--key", PRIVATE_KEY_FILE, "--out", env.subst(SIGNED_BIN)],
                   check=True, stdout=subprocess.DEVNULL)
    print("OTA: Signed firmware %s" % env.subst(SIGNED_BIN))


env.AddPostAction(FIRMWARE_BIN, sign_firmware)

if env.subst("$UPLOAD_PROTOCOL") == "espota":
    # Sign again in case the firmware was up to date and not built this run
    env.AddPreAction("upload", sign_firmware)
    with open(PASSWORD_FILE) as f:
        env.Append(UPLOADERFLAGS=["--auth=" + f.read().strip()])
    env.Replace(UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS -f ' + SIGNED_BIN)
