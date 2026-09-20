# Industrial Energy Edge Controller

An ESP32-based energy monitoring controller for measuring electrical usage, storing cumulative energy data, detecting faults, and publishing data through industrial and IoT interfaces.

## Features

- Voltage, current, real power, energy, and cost measurement
- Persistent energy and cost storage using ESP32 NVS
- FreeRTOS task-based firmware architecture
- Deterministic fault and recovery state machine
- Non-blocking Wi-Fi reconnection
- Read-only Modbus TCP telemetry
- MQTT telemetry with online/offline status
- Bounded offline telemetry buffering and controlled replay
- Optional Blynk telemetry
- Runtime diagnostics for heap, uptime, faults, reconnects, and task health
- Simulation input mode for software testing
- OTA firmware management with version and SHA-256 integrity checks

## Architecture

```mermaid
flowchart TD
    Sensors[Voltage / Current Sensors]
    EnergyTask[EnergyTask]
    Queues[FreeRTOS Queues]
    FaultTask[FaultTask]
    StorageTask[StorageTask]
    NetworkTask[NetworkTask]
    DiagnosticsTask[DiagnosticsTask]
    NVS[(ESP32 NVS)]
    MQTT[MQTT Broker]
    Buffer[Offline Buffer]
    Modbus[Modbus TCP Client]
    Blynk[Blynk Optional]
    OTA[OTA Manager]

    Sensors --> EnergyTask
    EnergyTask --> Queues
    Queues --> FaultTask
    Queues --> StorageTask
    Queues --> NetworkTask
    StorageTask --> NVS
    NetworkTask --> MQTT
    NetworkTask --> Buffer
    NetworkTask --> Modbus
    NetworkTask --> Blynk
    NetworkTask --> OTA
    DiagnosticsTask --> NetworkTask
```

### FreeRTOS Tasks

| Task | Responsibility |
|---|---|
| `EnergyTask` | Reads sensors, filters readings, and integrates energy |
| `FaultTask` | Evaluates readings and updates the fault state |
| `StorageTask` | Periodically saves cumulative values to NVS |
| `NetworkTask` | Handles Wi-Fi, MQTT, Modbus TCP, Blynk, and OTA services |
| `DiagnosticsTask` | Collects and prints runtime health information |

Each consumer receives its own one-element latest-value queue. This prevents a slow network or storage service from stopping measurement.

## Hardware Configuration

| Signal | ESP32 Pin | Calibration |
|---|---:|---:|
| Voltage | GPIO 35 | `162.7` |
| Current | GPIO 34 | `1.80` |

Configuration values are centralized in [include/config.h](include/config.h), including sampling periods, fault thresholds, MQTT settings, Modbus port, OTA settings, and buffer capacity.

## Energy Accounting

Energy is calculated from real power and elapsed time:

```text
energy_kWh += power_W * elapsed_hours / 1000
cost        = energy_kWh * tariff
```

Cumulative energy and cost are checkpointed to NVS. A no-load interval does not reset the stored totals.

## Fault Management

The controller uses the following state flow:

```text
NORMAL -> WARNING -> FAULT -> RECOVERY -> NORMAL
```

Current fault conditions include invalid sensor readings, overcurrent, and abnormal voltage. Fault debounce and recovery periods prevent unstable readings from causing immediate state changes.

See [docs/fault-state-machine.md](docs/fault-state-machine.md).

## MQTT Telemetry

Topics use the device ID from `config.h`:

```text
industrial-energy/<device-id>/telemetry
industrial-energy/<device-id>/status
industrial-energy/<device-id>/fault
industrial-energy/<device-id>/diagnostics
industrial-energy/<device-id>/ota
```

The MQTT service publishes retained `online` status and uses an `offline` Last Will message. When the broker is unavailable, telemetry is stored in a fixed-capacity circular buffer. The oldest record is dropped when the buffer is full, and records are replayed gradually after reconnect.

See [docs/mqtt-telemetry.md](docs/mqtt-telemetry.md).

## Modbus TCP

The built-in read-only Modbus TCP server listens on port `502` and supports function codes `0x03` and `0x04`.

Values use explicit integer scaling. For example:

```text
229.54 V -> 22954
1.253 A  -> 1253
276.4 W  -> 2764
```

See [docs/modbus-register-map.md](docs/modbus-register-map.md).

## OTA Firmware Management

`OtaManager` handles:

1. Firmware manifest parsing
2. Semantic version comparison
3. Firmware download
4. Image-size validation
5. SHA-256 integrity validation
6. Firmware installation and reboot
7. Post-update health and rollback confirmation hooks

SHA-256 provides integrity checking. Signed firmware, secure boot, and cryptographic authentication are not implemented.

See [docs/ota-update.md](docs/ota-update.md).

## Simulation Mode

`EnergySimulator` can provide `EnergyData` through the same interface as the real sensor path. Available scenarios include:

- Normal load
- No load
- Overcurrent
- Low voltage
- Invalid reading

## Project Structure

```text
include/     Public interfaces and configuration
src/         Firmware implementation
test/        Portable host-side tests
docs/        Architecture and interface documentation
platformio.ini
```

Important modules:

- `energy_logic`: portable filtering, integration, and cost calculation
- `energy_meter`: EmonLib sensor adapter
- `energy_store`: ESP32 Preferences/NVS storage
- `fault_manager`: fault detection and state transitions
- `network_manager`: Wi-Fi and optional Blynk handling
- `mqtt_service`: MQTT connection, payloads, status, and replay
- `telemetry_buffer`: bounded offline ring buffer
- `modbus_codec` and `modbus_server`: Modbus framing and TCP service
- `diagnostics`: runtime health collection
- `ota_manager`: firmware update control
- `version`: firmware version parsing and comparison

## Setup

1. Install PlatformIO.
2. Copy `include/secrets.example.h` to `include/secrets.h`.
3. Set Wi-Fi, Blynk, and MQTT credentials in `include/secrets.h`.
4. Build and upload the firmware:

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
```

Run the native tests with:

```bash
pio test -e native
```

`include/secrets.h` is ignored by Git. Do not place real credentials in tracked source files.
ESP32-based edge controller for **real-time electrical monitoring, energy accounting, persistent storage, and remote telemetry**.

The firmware measures AC voltage, current, and real power using **EmonLib**, calculates cumulative energy consumption and operating cost, stores critical state in ESP32 NVS, and publishes live telemetry to Blynk.

The project is structured so measurement, storage, and communication remain independent, making it easier to extend the controller with industrial protocols and reliability features.

---

## Features

- Real-time AC voltage and current measurement
- Real-power monitoring using **EmonLib**
- Configurable noise and low-load filtering
- Elapsed-time based energy accumulation
- Tariff and cumulative cost calculation
- Persistent storage using **ESP32 NVS**
- Automatic Wi-Fi reconnection
- Live **Blynk telemetry**
- Native tests for portable energy logic
- Credentials isolated from tracked source code

---

## System Architecture

```text
 Voltage Sensor (GPIO 35) ─┐
                           ├────> EmonLib
 Current Sensor (GPIO 34) ─┘         │
                                     ▼
                               Energy Meter
                                     │
                              Filter / Validate
                                     │
                                     ▼
                                Energy Sample
                                     │
                    ┌────────────────┼────────────────┐
                    │                │                │
                    ▼                ▼                ▼
                 Blynk            Serial         Energy Store
               Telemetry        Diagnostics           │
                                                       ▼
                                                    ESP32 NVS
```

The sensing and energy-accounting logic is kept separate from transport and persistence layers. This allows MQTT, Modbus TCP, RTOS tasks, and other industrial functionality to be added without redesigning the measurement pipeline.

---

## Measurement Configuration

| Parameter | Value |
|---|---:|
| Voltage input | GPIO `35` |
| Current input | GPIO `34` |
| Voltage calibration | `162.7` |
| Current calibration | `1.80` |
| Voltage cutoff | `50 V` |
| Current cutoff | `0.30 A` |
| Power cutoff | `5 W` |
| NVS checkpoint interval | `60 s` |
| Default tariff | `7.5 / kWh` |

Energy is accumulated from measured real power and actual elapsed time.

A missing load or filtered measurement does **not** reset previously accumulated energy or cost.

---

## Blynk Telemetry

The current firmware publishes live measurements using the following virtual pins:

| Virtual Pin | Metric |
|---|---|
| `V0` | Voltage |
| `V1` | Current |
| `V2` | Real Power |
| `V3` | Energy |
| `V4` | Cost |

---

## Project Output

![Industrial Energy Edge Controller](docs/results/Screenshot%202026-05-25%20235920.png)

*ESP32-based energy monitoring prototype with live telemetry.*

---

## Project Structure

```text
Industrial_Energy_Edge_Controller/
│
├── include/
│   ├── config.h
│   ├── energy_logic.h
│   ├── energy_meter.h
│   ├── energy_store.h
│   └── secrets.example.h
│
├── src/
│   ├── main.cpp
│   ├── energy_logic.cpp
│   ├── energy_meter.cpp
│   └── energy_store.cpp
│
├── test/
│   └── test_native/
│
├── docs/
│   └── results/
│
├── platformio.ini
└── README.md
```

### Main Components

**`main.cpp`**  
Coordinates measurement, Wi-Fi connectivity, Blynk telemetry, persistence, and the main firmware runtime.

**`energy_meter`**  
Handles electrical measurement and produces the measurement data used by the rest of the system.

**`energy_logic`**  
Contains filtering, energy accumulation, and portable calculation logic.

**`energy_store`**  
Handles persistent energy, cost, and tariff storage using ESP32 NVS.

**`config.h`**  
Contains hardware mappings, calibration constants, thresholds, and runtime configuration.

---

## Build and Setup

### 1. Clone the repository

```bash
git clone https://github.com/anupam-devcodes/Industrial_Energy_Edge_Controller.git
cd Industrial_Energy_Edge_Controller
```

### 2. Configure credentials

Copy:

```text
include/secrets.example.h
```

to:

```text
include/secrets.h
```

Add the Wi-Fi, Blynk, and other installation-specific credentials.

`include/secrets.h` is ignored by Git and should never be committed.

### 3. Build the firmware

```bash
pio run -e esp32dev
```

### 4. Upload to ESP32

```bash
pio run -e esp32dev -t upload
```

### 5. Open serial monitor

```bash
pio device monitor -b 115200
```

---

## Testing

Portable energy logic can be tested without ESP32 hardware using the PlatformIO native environment.

```bash
pio test -e native
```

This allows calculation and accumulation behavior to be verified independently from the hardware layer.

---

## Implementation Status

| Capability | Status |
|---|---|
| Voltage and current measurement | Implemented |
| Real-power measurement | Implemented |
| Signal filtering | Implemented |
| Energy accumulation | Implemented |
| Tariff and cost calculation | Implemented |
| ESP32 NVS persistence | Implemented |
| Wi-Fi reconnect handling | Implemented |
| Blynk telemetry | Implemented |
| Native logic tests | Implemented |
| MQTT runtime publishing | Integration pending |
| Modbus TCP publishing | Integration pending |
| Offline telemetry queue | Planned |
| FreeRTOS task separation | Planned |
| Watchdog health reporting | Planned |
| Simulation input mode | Planned |

MQTT and Modbus dependencies are already included in the project configuration, while their runtime integration remains part of the next development stage.

---

## Security

Deployment credentials are kept outside tracked source code using:

```text
include/secrets.h
```

Only the example configuration should be committed:

```text
include/secrets.example.h
```

Credentials exposed during earlier development should be rotated before deployment.

---

## Roadmap

The next development stage focuses on:

- MQTT telemetry publishing
- Bounded offline telemetry buffering
- Modbus TCP register exposure
- FreeRTOS-based task separation
- Watchdog and device-health reporting
- Simulation mode for development and testing

The existing measurement model will remain the core data source while these capabilities are added around it.

---

## Tech Stack

**ESP32 · C++ · Arduino Framework · PlatformIO · EmonLib · ESP32 NVS · Blynk**
