# Changelog

All notable changes to this project will be documented in this file.
The format is based on Keep a Changelog.

## [v1.4.0] - 2026-10-26
### Added
- Added a system log with timestamps and syslog severities, shown by the `syslog` command and at `/syslog`. It is kept across resets, e.g. by the watchdog, and marks each boot with the firmware version.
- Added the up time to the NTP synchronization message.
### Changed
- The messages of WiFi, NTP, mDNS, OTA, health and the web server are written to the system log. Only errors are printed to the serial console, the prompt of the CLI is shown after the startup messages.
- The web server is started with the other network services on the first WiFi connect.
- Disabled the `esp32_exception_decoder` monitor filter by default, as it delays the echo of the CLI.
- Updated to `libgeneric` v2.1.0 from the PlatformIO registry and `libparam` v1.2.0, pinned to its tag.
### Fixed
- Fixed a crash on boot without WiFi configured, e.g. on a new device.

## [v1.3.0] - 2026-10-25
### Added
- Added OTA firmware updates via network (`pio run -e nodemcu-32s-ota -t upload`), protected by a password and accepting signed firmware images only. Password and signing key are created in `secrets/` on the first build.
- Added a loop watchdog with 30s timeout and health checks restarting the device if the WiFi is lost for 10 minutes or the heap runs low.
- Added the reason of the last restart, the reset reason as text and the number of resets since power on to the info command and the root page.
- Added the `POST /reset` web endpoint to restart the device remotely.
- Added heap metrics, IP and DNS address, last NTP synchronization and OTA status to the info command and the root page.
- Added a fallback NTP server and log messages for mDNS and NTP.
### Changed
- WiFi handling is non-blocking now, reconnecting is left to the WiFi stack. mDNS, NTP and OTA are started once on the first connect.
- Disabled WiFi power save as it delayed incoming requests.
- The `networking off` command really disconnects the WiFi now.
- Parameter input rejects values that are too long instead of silently truncating them. DHCP accepts yes/true/on/1.
- Removed `lib_ldf_mode = deep+` as it fails to resolve the dependencies of arduino-esp32 3.x.
- Updated to `libCli` v4.7.0 and `IRremoteESP8266` v2.9.0, the own libraries are referenced by their default branch.
### Fixed
- Fixed the build failing with `Network.h: No such file or directory`.
- Fixed every IR transmission, reception and web request being delayed by 5s as long as NTP was not synchronized.
- Fixed an out of bounds read in `StringRingBuffer::peek()` after the log buffer wrapped around, affecting every 30th IR command.
- Fixed `WIFILED_ON`/`WIFILED_OFF` not using `WIFILED_PIN`.
- Fixed typos in the info output and missing commands in the help text.

## [v1.2.0] - 2026-04-06
### Added
- Added debug pin definitions to simplify diagnostics when needed.
- Added this changelog file to document project changes in a structured manner.
### Changed
- Enhanced the networking command to accept multiple input formats.
- Refactored mDNS and NTP initialization into `setup_wifi()`.
- Switched to `libCli` v4.3.0.
- Updated copyright years from 2025 to 2026 in multiple files.
### Fixed
- Added valid retransmission logic in `IRControl::transmit` to better match the behavior of handheld IR remotes.

## [v1.1.0] - 2025-07-26
### Added
- Added support for transmitting IR command sequences via the web API.
- Added the `/txseq` endpoint to execute multiple IR commands in a single request.
- Added configurable delays between commands in a transmitted sequence.
- Added documentation for the IR codes currently in use.
### Changed
- Refactored the project into separate libraries for IR control, web server handling, and shared utilities.
- Improved modularity through dedicated `IRControl` and `WebServerControl` classes.
- Expanded documentation for the new components.
- Improved validation and error handling for IR sequence parsing.
- Improved the loop termination condition in `strtoirtype()`.
### Fixed
- Corrected a typo in an error message.

## [v1.0.0] - 2024-03-02
Initial release of IR Gateway with core features for IR transmission, reception, web interface, and CLI.