# Industrial Energy Edge Controller

An ESP32-based energy monitoring controller for measuring electrical usage and providing energy data to industrial and IoT systems.

## Project Aim

The aim of this project is to build a reliable ESP32 energy-monitoring edge controller instead of a simple sensor dashboard. The controller measures electrical values, stores cumulative energy, detects abnormal conditions, and shares data with industrial and IoT systems.

Measurement continues independently from Wi-Fi, MQTT, Modbus TCP, Blynk, and OTA services. This keeps the core energy-monitoring function available even when a network service is unavailable.

## Features

- Voltage, current, power, energy, and cost measurement
- Persistent energy storage using ESP32 NVS
- ESP32 RTOS task-based architecture
- Fault detection and recovery states
- Wi-Fi and MQTT connectivity
- Offline MQTT telemetry buffering
- Read-only Modbus TCP telemetry
- Runtime diagnostics
- Simulation input mode
- OTA firmware management

## Architecture

```mermaid
flowchart TD
    Sensors[Voltage Sensor<br/>GPIO 35] --> Input[Energy Input]
    Current[Current Sensor<br/>GPIO 34] --> Input
    Simulator[Simulation Input] -. optional .-> Input
    Input --> Energy[EnergyTask]
    Energy --> Measure[EnergyData<br/>Voltage / Current / Power / Energy / Cost]
    Measure --> EnergyQueues[Latest-value ESP32 RTOS Queues]

    EnergyQueues --> FaultTask[FaultTask]
    EnergyQueues --> StorageTask[StorageTask]
    EnergyQueues --> NetworkTask[NetworkTask]

    FaultTask --> FaultManager[FaultManager]
    FaultManager --> FaultState[SystemState<br/>NORMAL / WARNING / FAULT / RECOVERY]
    FaultState --> Snapshot[Shared Snapshot]
    Measure --> Snapshot

    StorageTask --> NVS[(ESP32 Preferences / NVS)]

    NetworkTask --> WiFi[Wi-Fi Manager]
    NetworkTask --> MQTTService[MQTT Service]
    NetworkTask --> ModbusService[Modbus TCP Server]
    NetworkTask --> Blynk[Blynk Optional]
    NetworkTask --> OTAManager[OTA Manager]

    MQTTService --> MQTTBroker[MQTT Broker]
    MQTTService --> OfflineBuffer[Bounded Offline Telemetry Buffer]
    OfflineBuffer --> MQTTService

    ModbusService --> ModbusClient[Industrial Modbus TCP Client]
    Blynk --> BlynkCloud[Blynk Service]
    OTAManager --> Firmware[Versioned Firmware Update]

    DiagnosticsTask[DiagnosticsTask] --> Snapshot
    DiagnosticsTask --> RuntimeHealth[Runtime Diagnostics]
    RuntimeHealth --> MQTTService
    RuntimeHealth --> Serial[Serial Health Output]

    Snapshot -. mutex-protected .- NetworkTask
    Snapshot -. mutex-protected .- DiagnosticsTask
```

## Hardware

| Signal | ESP32 Pin |
|---|---:|
| Voltage input | GPIO 35 |
| Current input | GPIO 34 |

Sensor calibration and system settings are defined in [include/config.h](include/config.h).

## Interfaces

### MQTT

```text
industrial-energy/<device-id>/telemetry
industrial-energy/<device-id>/status
industrial-energy/<device-id>/fault
industrial-energy/<device-id>/diagnostics
industrial-energy/<device-id>/ota
```

Telemetry is buffered locally when MQTT is unavailable and replayed after reconnect.

### Modbus TCP

The controller provides read-only Modbus TCP telemetry on port `502`.

See [docs/modbus-register-map.md](docs/modbus-register-map.md).

## Configuration

Create the local credentials file:

```bash
copy include/secrets.example.h include/secrets.h
```

Set Wi-Fi, Blynk, and MQTT credentials in `include/secrets.h`. This file is ignored by Git.

## Build and Upload

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
```

Run native tests with:

```bash
pio test -e native
```

## Project Structure

```text
include/     Headers and configuration
src/         Firmware source code
test/        Native tests
docs/        Architecture and interface documentation
```

Detailed documentation:

- [Architecture](docs/architecture.md)
- [Fault state machine](docs/fault-state-machine.md)
- [MQTT telemetry](docs/mqtt-telemetry.md)
- [Modbus register map](docs/modbus-register-map.md)
- [OTA updates](docs/ota-update.md)
