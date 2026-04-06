# Changelog

All notable changes to this project will be documented in this file.
The format is based on Keep a Changelog.

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