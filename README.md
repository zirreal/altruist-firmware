# Altruist Firmware

Firmware for the Altruist environmental sensor station, built on ESP32-C6.

## Architecture Overview

In the Robonomics sensors architecture, the Altruist plays two roles:

- **Datalog reporting**: Signs and sends measurement datalogs directly to the Robonomics parachain via RPC node (every 10 minutes).
- **Connectivity reporting**: Signs and sends measurement data to Sensors Connectivity Provider nodes via HTTP POST on port 65 (every 30 seconds).

```
Altruist (ESP32-C6)
  |
  |-- Signed extrinsic --> Robonomics Parachain (Polkadot)
  |                            |
  |                  RoSeMAN indexes --> MongoDB --> sensors.social
  |
  +-- Signed msg HTTP:65 --> Sensors Connectivity Provider
        |-- Real-time: IPFS pubsub --> Robonomics dApp
        +-- Batch: IPFS pin --> datalog hash --> Parachain
```

An ED25519 keypair is generated on first boot and stored in SPIFFS (`/config.json`). This identity is used to sign both datalog extrinsics and connectivity messages.

The connectivity server pool is defined in `robonomics_servers.h` (currently 3 servers). On startup, the device polls all servers and picks one where it is already registered, or the least loaded one.

Hardware reset (GPIO7) clears WiFi credentials and password but preserves the Robonomics identity.

## Hardware Variants

### Altruist Urban

Outdoor station (ESP32-C6 or ESP32-C3). Provides environmental and air quality measurements. **ESP32-C6:** discovered by Insight via mDNS (`altruist._tcp`). **ESP32-C3:** no mDNS (flash budget) — pair Insight with **custom Urban IP** in config; open the device by LAN IP in a browser.

### Altruist Insight

Indoor station (ESP32-C6) with display and QR code support. Can aggregate data from nearby Urban devices over the local network.

## Supported Sensors

| Sensor               | Measurement                                     |
| -------------------- | ----------------------------------------------- |
| SDS011               | PM2.5, PM10                                     |
| BMx280 (BMP/BME 280) | Temperature, humidity, pressure                 |
| BME680               | Temperature, humidity, pressure, gas resistance |
| SCD4x (SCD40/SCD41)  | CO2, temperature, humidity                      |
| RadSens              | Radiation (counts per minute)                   |
| I2S microphone       | Noise level (dBA)                               |
| GPS (Neo-6M)         | Latitude, longitude                             |
| HTTP Altruist sensor | Data from linked Urban devices                  |

## Building and Flashing

The project uses [PlatformIO](https://platformio.org/install/cli). Build environments are defined in `platformio.ini`.

Build for a specific target:

```bash
pio run -e esp32c6_urban_en
pio run -e esp32c6_inside_en
```

Flash:

```bash
pio run -e esp32c6_urban_en --target upload
```

Available release environments: `esp32c3_urban_en`, `esp32c3_urban_ru`, `esp32c6_urban_en`, `esp32c6_urban_ru`, `esp32c6_inside_en`, and `esp32c6_inside_ru`. ESP32-C6 technical debug environments use the `_debug` suffix and do not produce publishable firmware artifacts.

Release builds produce channel-aware files in `builds/`: Stable uses names such as `latest32c6urb_en.bin`, while Testing uses `latest32c6urb_en_testing.bin`. During the webflasher transition, Testing builds also produce matching `_dev.bin` aliases.

Stable firmware keeps the base version (for example `R-URB_2026-06.1`). Testing firmware includes its source revision (for example `R-URB_2026-06.1-testing+7445b03`). The startup UART log and status page also expose the channel, source commit, device model, ESP target, language, and build profile.

OTA updates remain on the compile-time channel: Stable firmware requests the normal language artifact, while Testing firmware requests the corresponding `_testing.bin` artifact. Changing the runtime log level or legacy `use_beta` configuration cannot switch channels.

## Configuration

On first boot (or after reset), the device starts in Access Point mode. Connect to its WiFi network and open the configuration page to set:

- WiFi credentials
- GPS coordinates
- Sensor enable/disable
- API endpoints

After configuration, the device restarts and connects to the specified WiFi network. The web UI remains available on the local network for reconfiguration.

## Button Controls

### Insight

- `UP` short press - previous screen
- `DOWN` short press - next screen
- `UP` / `DOWN` short press on **Graphs** screen - switch graph (at edges switches screen). Long press changes screen
- `SET` long press - sleep
- `SET` + `DOWN` long press (4s) - reset WiFi configuration
- `SET` + `DOWN` pressed while powering on - reset all configuration

### Urban (ESP32-C6 with hardware UI)

**Reset button (GPIO7)**

- **While powering on** (hold before/at power-on) — factory reset: deletes the full `config.json` including the Robonomics identity (same as Insight `SET` + `DOWN` at boot).
- **While running** — hold for more than 10 seconds, then release: clears Wi‑Fi credentials and the web UI password only; Robonomics identity is preserved and the device reboots into the setup captive portal. Boot-time and runtime actions do not overlap — a power-on hold is consumed as factory reset only.
- LEDs turn blue briefly while the runtime Wi‑Fi reset is applied.

**Status LEDs** (NeoPixel ring on ESP32-C6 Urban; both pixels show the same color)

| Color            | Meaning                                                                         |
| ---------------- | ------------------------------------------------------------------------------- |
| **Green**        | Normal operation — Wi‑Fi connected and on-chain Robonomics datalog is healthy.  |
| **Blue**         | Setup mode — no saved Wi‑Fi yet, or the captive configuration portal is active. |
| **Blue**         | Also shown while an on-chain datalog transmission is in progress.               |
| **Green** (~3 s) | Last datalog send succeeded (brief flash after transmission).                   |
| **Red** (~3 s)   | Last datalog send failed (brief flash after transmission).                      |
| **Red** (steady) | Wi‑Fi disconnected or datalog unhealthy for more than 10 minutes.               |

Map/connectivity HTTP errors do not drive the steady red state — LED status reflects **Robonomics datalog** health, not the sensors map POST.

LED indication can be disabled in the web configuration.

## Contributing

All development changes should be submitted as pull requests against the **beta** branch. The **master** branch reflects the current release firmware.

To add a Connectivity Robonomics Server, fork this repository and edit `robonomics_servers.h`. Add your server:

```c
{"<server_address>", REGION_XX}
```

Available regions: `REGION_GLOBAL`, `REGION_EU`, `REGION_AS`, `REGION_AF`, `REGION_AU`, `REGION_NA`, `REGION_SA`.
