# Industrial Energy Edge Controller

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