# Changelog

All notable changes to the Altruist Firmware project will be documented in this file.

---

## [R_2026-03](https://github.com/airalab/altruist-firmware/releases/tag/R_2026-03) — 2026-03-16

### New Features

- **Night-only Analytics screen (Insight/e-ink)** — redesigned Analytics into a single smartwatch-style nightly recovery page with a large central score ring and compact metrics table.

### Improvements

- **Night score methodology table** — added side-by-side methodology scores with on-screen legend (`C = Conservative`, `B = Biohacking`) to make score interpretation explicit.
- **Analytics storage and rendering path** — analytics now uses night-focused 24h/hourly data flow only.
- **Analytics NVS-first mode** — analytics persistence moved to a compact 48-hour hourly NVS ring, and night rendering now reads NVS/RAM history directly.
- **Storage simplification for Analytics** — removed analytics SD cache/migration dependencies from runtime; SD is no longer required to open Analytics screen.
- **Analytics DEV observability** — added DEV logs for persistence state, category/metric coverage, and save telemetry (first/last save time, save count, last reason, save policy).
- **Night scoring methodology update** — switched Conservative/Biohacking scoring to sleep-impact formulas per metric (CO2, PM2.5, noise, temperature, humidity) with score mapping `100 + impact*2`.
- **Night score aggregation update** — final Conservative/Biohacking scores now use summed model impact (`100 + total_impact*2`) to match the strict methodology examples.
- **Night summary line refresh** — replaced generic footer with a single-line human-readable summary for both models (`score + grade + sleep impact`).
- **Night data readiness state** — when not enough night hours are collected, Analytics shows explicit "collecting data" status (no table box) instead of a misleading score.
- **Night analytics qr code** - added qr code with sensor map for more thorough and proper analytics.
- **Background analytics ingest** — hourly analytics ingestion was decoupled from Analytics screen rendering and now runs in the sensor worker, so night history keeps updating even when Analytics screen is not opened.
- **Hourly persistence policy** — persistence now targets hourly boundaries with catch-up behavior after reboot/missed windows, improving next-morning report reliability.
- **Morning analytics recompute window** — night report selection now supports progressive hourly refresh between `06:00` and configured night end hour (default `10:00`) instead of waiting for a single daily switch.
- **Analytics screen priority window (Insight)** — analytics can auto-open as default between `06:00` and `12:00`, and auto-return to `MAIN` after the window ends while preserving manual navigation.
- **Typography and units upgrade (e-ink analytics/main)** — added glyph support for `°`, `µ`, and superscripts; updated UI units to full forms (`°C`, `µg/m³`).
- **LED resilience under mutex contention (Insight)** — added LED mutex diagnostics and a guarded daytime fallback that forces neutral LED ON state when updates are blocked too long.
- **Configurable LED night schedule (Insight)** — added web-configurable `LED off hour` / `LED on hour` (defaults `00` and `06`) and switched logic from hardcoded quiet hours.
- **Main screen lock-time reduction (Insight)** — removed serialize/deserialize JSON path from display refresh; main screen now uses typed cached snapshot extracted under a short mutex hold.
- **Network/API hardening on unstable WiFi** — guarded API send paths and skip send cycle when WiFi remains disconnected after reconnect attempt.
- **Crash diagnostics in status page** — added reset reason code, last crash section, previous uptime, and previous free heap.

### Bug Fixes

- **SD rollup path handling** — fixed CSV open paths in rollup builders to always use absolute paths (`/...`) and avoid VFS `does not start with /` errors.
- **SD-card robustness and retention** — extended retention worker and SD mutex usage for logging/cleanup/read paths; retention now covers raw graph files, analytics daily rollups for dev, and boot diagnostics.
- **CSV logging regression after analytics refactor** — fixed one-shot `jsonUpdated()` flag consumption so SD CSV writes and analytics ingest can run together without dropping graph data.

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
