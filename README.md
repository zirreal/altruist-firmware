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

Outdoor station (ESP32-C6). Provides environmental and air quality measurements. Discovered by Insight devices via mDNS (`altruist._tcp`).

### Altruist Insight

Indoor station (ESP32-C6) with display and QR code support. Can aggregate data from nearby Urban devices over the local network.

## Supported Sensors

| Sensor | Measurement |
|--------|-------------|
| SDS011 | PM2.5, PM10 |
| BMx280 (BMP/BME 280) | Temperature, humidity, pressure |
| BME680 | Temperature, humidity, pressure, gas resistance |
| SCD4x (SCD40/SCD41) | CO2, temperature, humidity |
| RadSens | Radiation (counts per minute) |
| I2S microphone | Noise level (dBA) |
| GPS (Neo-6M) | Latitude, longitude |
| HTTP Altruist sensor | Data from linked Urban devices |

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

Available environments: `esp32c6_urban_en`, `esp32c6_urban_ru`, `esp32c6_inside_en`, `esp32c6_inside_ru` (plus `_dev` variants with debug output).

## Configuration

On first boot (or after reset), the device starts in Access Point mode. Connect to its WiFi network and open the configuration page to set:

- WiFi credentials
- GPS coordinates
- Sensor enable/disable
- API endpoints

After configuration, the device restarts and connects to the specified WiFi network. The web UI remains available on the local network for reconfiguration.

## Button Controls

### Insight

- `UP` or `DOWN` short press -- show or change graphs screen
- `SET` short press -- show main screen or refresh
- `SET` + `DOWN` long press (4s) -- reset WiFi configuration
- `SET` + `DOWN` pressed while powering on -- reset all configuration

### Urban

- `SET` long press (4s) -- reset WiFi configuration
- `SET` pressed while powering on -- reset all configuration

## Contributing

All development changes should be submitted as pull requests against the **beta** branch. The **master** branch reflects the current release firmware.

To add a Connectivity Robonomics Server, fork this repository and edit `robonomics_servers.h`. Add your server:

```c
{"<server_address>", REGION_XX}
```

Available regions: `REGION_GLOBAL`, `REGION_EU`, `REGION_AS`, `REGION_AF`, `REGION_AU`, `REGION_NA`, `REGION_SA`.
