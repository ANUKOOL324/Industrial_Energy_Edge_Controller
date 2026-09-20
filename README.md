````markdown
# Industrial Energy Edge Controller

ESP32-based industrial edge controller for **real-time electrical monitoring, energy accumulation, tariff calculation, persistent storage, and remote telemetry**.

The firmware measures AC voltage, current, and real power using **EmonLib**, calculates cumulative energy consumption and operating cost, persists critical state using ESP32 NVS, and publishes live telemetry to a Blynk dashboard.

The project is structured around independent **measurement, persistence, and communication layers**, allowing additional industrial protocols and reliability features to be integrated without turning the firmware into a monolithic sketch.

---

## Overview

The Industrial Energy Edge Controller is designed as a lightweight monitoring node positioned close to electrical equipment.

It continuously tracks:

- AC Voltage
- AC Current
- Real Power
- Energy Consumption
- Tariff
- Estimated Energy Cost

Measurements are processed locally on the ESP32. Energy and cost remain cumulative even when the monitored load becomes inactive or temporarily disconnects.

The firmware separates instantaneous measurements from persistent energy accounting, making the system suitable for further development into a more complete industrial edge monitoring platform.

---

## System Architecture

```text
                        AC Load
                           │
               ┌───────────┴───────────┐
               │                       │
        Voltage Sensor          Current Sensor
               │                       │
            GPIO 35                  GPIO 34
               │                       │
               └───────────┬───────────┘
                           │
                           ▼
                        EmonLib
                           │
                           ▼
                     Energy Meter
                           │
                  Measurement Filtering
                           │
                           ▼
                      EnergySample
                           │
            ┌──────────────┼──────────────┐
            │              │              │
            ▼              ▼              ▼
         Blynk          Serial        Energy Store
       Telemetry      Diagnostics          │
                                           ▼
                                        ESP32 NVS
                                           │
                                  Persistent Energy,
                                    Cost & Tariff
```

The core measurement pipeline is intentionally independent from communication protocols.

This allows MQTT, Modbus TCP, watchdog monitoring, simulation inputs, and RTOS tasks to consume the same measurement data without changing the underlying sensing implementation.

---

## Core Features

### Electrical Measurement

Electrical measurements are acquired using **EmonLib** with the existing voltage and current sensor path.

Current hardware mapping:

```text
Voltage Sensor  -> GPIO 35
Current Sensor  -> GPIO 34
```

Current calibration values:

```text
Voltage Calibration : 162.7
Current Calibration : 1.80
```

Calibration and runtime constants are maintained separately from the application logic.

---

### Measurement Filtering

Very small readings caused by sensor noise are filtered before being processed.

Current thresholds:

```text
Voltage Cutoff : 50 V
Current Cutoff : 0.30 A
Power Cutoff   : 5 W
```

Measurements below these thresholds are treated as inactive load values.

This prevents small ADC fluctuations and sensor noise from incorrectly increasing cumulative energy consumption.

---

### Energy Accumulation

Energy consumption is calculated using measured real power and actual elapsed time.

```text
Energy (kWh) = Power (W) × Time (hours) / 1000
```

Using elapsed time instead of assuming a perfectly fixed loop interval makes energy accumulation more resilient to normal execution delays and network activity.

A missing or filtered load does **not** reset previously accumulated energy.

---

### Tariff and Cost Calculation

The controller tracks estimated energy cost using:

```text
Cost = Energy (kWh) × Tariff
```

Default tariff:

```text
7.5 / kWh
```

Tariff handling is kept separate from sensor acquisition so pricing can be modified without changing the measurement pipeline.

---

## Persistent Storage

Critical cumulative values are stored using **ESP32 Non-Volatile Storage (NVS)**.

Persisted values include:

```text
Energy
Cost
Tariff
```

Instead of continuously writing to flash, the controller checkpoints its accumulated state periodically.

Current checkpoint interval:

```text
60 seconds
```

This reduces unnecessary flash writes while protecting accumulated measurements against unexpected restarts and power interruptions.

---

## Connectivity

### Wi-Fi

The ESP32 operates using the Wi-Fi station interface.

The firmware does not depend on a blocking startup retry loop. Network reconnection is handled independently so temporary Wi-Fi loss does not stop the measurement pipeline.

---

### Blynk Telemetry

The current runtime publishes measurements to Blynk using the following virtual-pin mapping:

| Virtual Pin | Measurement |
|-------------|-------------|
| `V0` | Voltage |
| `V1` | Current |
| `V2` | Real Power |
| `V3` | Energy |
| `V4` | Cost |

This mapping maintains compatibility with the existing dashboard implementation.

---

## Project Output

The following image shows the prototype and live telemetry output.

> Replace the filename below with the exact filename of your image inside the `docs` directory.

```markdown
![Industrial Energy Edge Controller](docs/results/Screenshot%202026-05-25%20235920.png)
```

use:

```markdown
![Industrial Energy Edge Controller](docs/energy-controller-result.png)
```

### Prototype Result

![Industrial Energy Edge Controller](docs/energy-controller-result.png)

---

## Repository Structure

```text
Industrial_Energy_Edge_Controller/
│
├── include/
│   ├── config.h
│   ├── energy_logic.h
│   ├── energy_meter.h
│   ├── energy_store.h
│   ├── secrets.example.h
│   └── secrets.h
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
│   └── energy-controller-result.png
│
├── platformio.ini
└── README.md
```

### `main.cpp`

Coordinates firmware startup and runtime behavior including measurement, connectivity, persistence, and telemetry.

### `energy_meter`

Handles electrical measurement and produces the common measurement representation used by the rest of the firmware.

### `energy_logic`

Contains portable energy-processing logic including filtering and energy accumulation.

Keeping this logic independent from ESP32-specific APIs allows it to be tested using the native PlatformIO environment.

### `energy_store`

Provides persistent storage for cumulative energy, cost, and tariff values using ESP32 NVS.

### `config.h`

Contains hardware mapping, calibration values, thresholds, timing intervals, and other runtime configuration.

### `secrets.h`

Contains installation-specific credentials and is intentionally excluded from version control.

---

## Build Environment

The project uses **PlatformIO** with the ESP32 Arduino framework.

### Requirements

- ESP32 development board
- VS Code
- PlatformIO
- Voltage sensing circuit
- Current sensing circuit
- Wi-Fi connection

---

## Installation

Clone the repository:

```bash
git clone https://github.com/anupam-devcodes/Industrial_Energy_Edge_Controller.git
```

Move into the project:

```bash
cd Industrial_Energy_Edge_Controller
```

Copy:

```text
include/secrets.example.h
```

to:

```text
include/secrets.h
```

Then configure the deployment credentials required by the installation.

These can include:

```text
Wi-Fi SSID
Wi-Fi Password
Blynk Credentials
MQTT Configuration
```

`include/secrets.h` is ignored by Git and should never be committed.

---

## Build and Flash

Compile the firmware:

```bash
pio run -e esp32dev
```

Upload it to the ESP32:

```bash
pio run -e esp32dev -t upload
```

Open the serial monitor:

```bash
pio device monitor -b 115200
```

---

## Testing

Portable measurement and energy logic can be tested independently of ESP32 hardware.

Run the native test suite with:

```bash
pio test -e native
```

Separating portable calculation logic from hardware-specific code allows energy accounting behavior to be validated without repeatedly flashing the microcontroller.

---

## Current Implementation Status

| Component | Status |
|-----------|--------|
| Voltage Measurement | Implemented |
| Current Measurement | Implemented |
| Real Power Measurement | Implemented |
| Signal Filtering | Implemented |
| Energy Accumulation | Implemented |
| Tariff Calculation | Implemented |
| Cost Calculation | Implemented |
| ESP32 NVS Persistence | Implemented |
| Wi-Fi Connectivity | Implemented |
| Blynk Telemetry | Implemented |
| Native Logic Tests | Implemented |
| MQTT Dependencies / Configuration | Available |
| MQTT Runtime Publishing | Integration Pending |
| Modbus TCP Dependency | Available |
| Modbus TCP Runtime Publishing | Integration Pending |
| Offline Telemetry Queue | Planned |
| FreeRTOS Task Separation | Planned |
| Watchdog Health Monitoring | Planned |
| Simulation Input Mode | Planned |

---

## Industrial Expansion

The architecture is designed so additional industrial interfaces can operate on the same measurement model.

```text
                         EnergySample
                              │
           ┌──────────────────┼──────────────────┐
           │                  │                  │
           ▼                  ▼                  ▼
        Blynk               MQTT             Modbus TCP
                              │                  │
                              ▼                  ▼
                       Offline Queue       Register Mapping
           │
           ▼
    Local Diagnostics
```

The next engineering layer can introduce:

- MQTT telemetry publishing
- Bounded offline MQTT buffering
- Modbus TCP register publishing
- FreeRTOS task separation
- Watchdog health reporting
- Simulation input mode
- Device health telemetry
- Improved network-failure handling

The objective is to add these capabilities around the existing measurement model rather than coupling communication protocols directly to sensor acquisition.

---

## Security

Deployment credentials are separated from tracked source code.

The local file:

```text
include/secrets.h
```

should remain excluded from Git.

Only the template should be committed:

```text
include/secrets.example.h
```

Any credentials exposed in previous development revisions should be rotated before deploying the controller.

---

## Technology Stack

| Layer | Technology |
|------|------------|
| Microcontroller | ESP32 |
| Language | C++ |
| Framework | Arduino |
| Build System | PlatformIO |
| Electrical Measurement | EmonLib |
| Persistence | ESP32 NVS |
| Dashboard / Telemetry | Blynk |
| MQTT | PubSubClient |
| Industrial Communication | Modbus TCP |
| Testing | PlatformIO Native Tests |

---

## Engineering Direction

The project is evolving from a connected energy-monitoring prototype toward a modular **industrial edge energy controller**.

The core measurement model remains isolated from transport and persistence concerns so communication protocols, reliability mechanisms, and industrial interfaces can be added incrementally without redesigning the sensor-processing pipeline.
````
