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
# OTA pre script: Creates the OTA password and the signing key in secrets/ if
# missing and passes the password and the public key to the firmware.

import os
import secrets
import subprocess

Import("env")

SECRETS_DIR = os.path.join(env.subst("$PROJECT_DIR"), "secrets")
PASSWORD_FILE = os.path.join(SECRETS_DIR, "ota_password")
PRIVATE_KEY_FILE = os.path.join(SECRETS_DIR, "ota_signing_key.pem")
PUBLIC_KEY_FILE = os.path.join(SECRETS_DIR, "ota_public_key.pem")
PUBLIC_KEY_HEADER = os.path.join(SECRETS_DIR, "ota_public_key.h")


def bin_signing(*args):
    tool = os.path.join(
        env.PioPlatform().get_package_dir("framework-arduinoespressif32"),
        "tools", "bin_signing.py")
    subprocess.run([env.subst("$PYTHONEXE"), tool] + list(args),
                   check=True, stdout=subprocess.DEVNULL)


os.makedirs(SECRETS_DIR, mode=0o700, exist_ok=True)

if not os.path.isfile(PASSWORD_FILE):
    with open(os.open(PASSWORD_FILE, os.O_WRONLY | os.O_CREAT, 0o600), "w") as f:
        f.write(secrets.token_urlsafe(18) + "\n")
    print("OTA: Created new password in %s" % PASSWORD_FILE)

if not os.path.isfile(PRIVATE_KEY_FILE):
    bin_signing("--generate-key", "ecdsa-p256", "--out", PRIVATE_KEY_FILE)
    os.chmod(PRIVATE_KEY_FILE, 0o600)
    print("OTA: Created new signing key in %s" % PRIVATE_KEY_FILE)
    print("OTA: Devices running firmware with another key have to be flashed "
          "via USB once.")

if (not os.path.isfile(PUBLIC_KEY_HEADER) or
        os.path.getmtime(PUBLIC_KEY_HEADER) < os.path.getmtime(PRIVATE_KEY_FILE)):
    bin_signing("--extract-pubkey", PRIVATE_KEY_FILE, "--out", PUBLIC_KEY_FILE)

with open(PASSWORD_FILE) as f:
    password = f.read().strip()

env.Append(
    CPPDEFINES=["UPDATE_SIGN", ("OTA_PASSWORD", env.StringifyMacro(password))],
    CPPPATH=[SECRETS_DIR])
