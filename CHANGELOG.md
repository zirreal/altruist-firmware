# Changelog

All notable changes to the Altruist Firmware project will be documented in this file.

---

## [R_2026-02.2](https://github.com/airalab/altruist-firmware/releases/tag/R_2026-02.2) — 2026-02-24

## Summary

- Reduce panic risk during unstable WiFi by guarding HTTP/API sends.
- Add SD-card retention cleanup and synchronize SD access with a global mutex.
- Improve crash diagnostics in web status by showing reset code + last crash section.

## What Changed

### 1) API/WiFi panic hardening

- In `sensorAndAPIWorker`, if WiFi remains disconnected after reconnect attempt, skip API send for that cycle.
- Added WiFi-connected guards in:
  - `RobonomicsHTTPAPI`
  - `CustomHTTPAPI`
- Avoided risky transport-error body reads on failed connections by using safer error reporting.

### 2) SD-card robustness and retention

- Added SD retention policy:
  - keep sensor CSV history for `14 days`
  - keep only latest `100` boot diagnostic files
- Added `sdRetentionWorker` periodic cleanup task (every 6h check window).
- Added global recursive SD mutex (`sdCardLock`/`sdCardUnlock`) and applied locking across SD logging/cleanup/read paths.
- Added SD locking around graph file discovery and runtime log rotation paths to reduce concurrent SD access issues.

### 3) Better crash visibility in Web UI

- Status page now includes:
  - reset reason code
  - last crash section (if saved)
  - previous uptime before reset
  - previous free heap before reset

## Why

Field logs showed:

- Intermittent panic resets around network/API activity.
- Growing SD diagnostic/data files over time.
- Status page was too generic for fast root-cause triage.

These changes target reliability and observability without changing core feature behavior.

## Notes

- This PR is stability-focused and safe for hotfix release.
- Retention values are currently hardcoded (`14d` sensor CSV, `100` boot logs) and can be made configurable later.

## [R_2026-02.1](https://github.com/airalab/altruist-firmware/releases/tag/R_2026-02.1) — 2026-02-20

### Bug Fixes

- **Fix e-paper display freeze** — display could get permanently stuck when the EPD busy pin failed to release; the driver now detects the timeout and triggers immediate hardware reset + full refresh recovery
- **Reduce EPD busy timeout** — decreased from 30s to 10s for faster stuck-state detection (normal operations complete in 2–4s)
- **Extend EPD watchdog to all screens** — the periodic 25-minute recovery watchdog now runs on all navigable screens (GRAPHS, SENSOR_MAP, SETTINGS), not only MAIN

---

## [R_2026-02](https://github.com/airalab/altruist-firmware/releases/tag/R_2026-02) — 2026-02-20

### New Features

- **Altruist Insight (Inside) support** — full support for the new Altruist Insight board variant with ESP32-C6, including e-paper display, SD card logging, LED strip, and button controls
- **Publish to Map** — new configurable data sharing feature allowing users to select which sensor values (temperature, humidity, pressure, CO2, PM, noise) are published to the public sensors map
- **Russian language support** — full Russian localization for firmware UI, display screens, and web interface
- **Language switch** — ability to choose between English and Russian firmware builds
- **New display screens and navigation** — completely redesigned e-paper display with Main, Graphs, Sensor Map, and Settings screens; new icon set and right-side navigation stack
- **SD card data graphs** — graph screen with historical sensor data read from SD card CSV files, supporting 12-hour rolling window with today + yesterday data

### Improvements

- **Redesigned main screen layout** — updated Urban 2-column layout with QR code, device IP, and Robonomics addresses
- **Improved buttons logic** — short press and long press actions for UP, SET, DOWN buttons; long DOWN press enters sleep mode
- **LED enhancements** — extended LED indicators for more sensor values; time-based night dimming schedule (22:00–06:00); updated noise LED ranges
- **Partial e-paper display refresh** — 10 partial refreshes between full refreshes for faster screen updates; EPD watchdog re-init every 25 minutes
- **WiFi AP password protection** — access point mode now requires a password (default: `123456789`) instead of being open
- **Improved OTA updates** — two-stage OTA with dual download hosts, progress percentage display, MD5 verification, and retry logic
- **Separated dev/prod logging** — different log levels for development and production firmware builds
- **Custom Altruist address in config** — ability to set a custom Altruist Urban device address

### Bug Fixes

- Fix memory leak on display refresh (#52)
- Fix negative temperature display
- Fix "lost Urban for Insight" bug
- Fix WiFi connectivity issue
- Fix QR code rendering
- Fix SD card data display and screen switch updates

### Build & Infrastructure

- Build configurations for all ESP32-C6 variants: Urban EN/RU, Insight EN/RU (plus dev builds)
- Updated pin mappings for Insight boards
- DEV postfix added to development firmware filenames

---

## [R_2025-06](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-06) — 2025-06-23

- Fix chipid
- Add Restart Reason to device status page
- Handle Webserver in separate task
- Manually reconnect to WiFi
- Add Custom API

---

## [R_2025-04](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-04) — 2025-04-10

- Fix mobile UI styles

---

## [R_2025-03](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-03) — 2025-03-26

- Add possibility to remove only wifi configuration
- Show new sensor IP address after first configuration
- Add local domain to the config
- Add SCD4x Sensor
- Add RadSens Sensor
- Show location on the map

---

## [R_2025-02.2](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-02.2) — 2025-02-26

- Fix noise calculations
- Change web server footer
- Fix private key saving

---

## [R_2025-02.1](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-02.1) — 2025-02-24

- Automatically choose connected sensors
- Code refactoring
- Classes for sensors and APIs
- Change Web interface

---

## [R_2025-02](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-02) — 2025-02-12

- Robonomics public node hostname can be set in config
- Changes for Home Assistant discovery service
- Independent timeout for Datalog sending
- Fix BME280 I2C sensor
- Signed requests to Sensors Connectivity

---

## [R_2025-01](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-01) — 2025-01-13

- Split GPS fields in config
- Add auto update for ESP32
- Build firmware for ESP32-C3
- Generate and save private key for Robonomics Account
- Send Robonomics Datalog Extrinsic with sensors data

---

## [2024.09](https://github.com/airalab/altruist-firmware/releases/tag/2024.09) — 2024-09-18

- Send noise every 5 seconds

---

## [R_2024-06](https://github.com/airalab/altruist-firmware/releases/tag/R_2024-06) — 2024-06-13

- Added Noise meter sensor PCBA
