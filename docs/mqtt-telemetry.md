# MQTT Telemetry

MQTT is a NetworkTask service. `EnergyTask` produces `EnergyData`; it does not know that MQTT exists. This keeps sensing and energy accounting running when Wi-Fi or the broker is unavailable.

## Configuration

| Setting | Location | Purpose |
|---|---|---|
| Broker address and port | `config::mqttBroker`, `config::mqttPort` | Broker endpoint |
| Device ID | `config::mqttDeviceId` | Topic namespace |
| Publish period | `config::mqttPublishPeriodMs` | Live telemetry rate |
| Reconnect period | `config::mqttReconnectIntervalMs` | Minimum time between attempts |
| Buffer capacity | `config::telemetryBufferCapacity` | Maximum offline records |
| MQTT username/password | ignored `include/secrets.h` | Optional broker credentials |

PubSubClient is declared in [platformio.ini](../platformio.ini). It is not downloaded as part of the current offline validation.

## Topics

The default device ID is `meter01`:

```text
industrial-energy/meter01/telemetry
industrial-energy/meter01/status
industrial-energy/meter01/fault
industrial-energy/meter01/diagnostics
industrial-energy/meter01/ota
```

## Payloads

Telemetry contains only values produced by the firmware:

```json
{
  "voltage": 229.54,
  "current": 1.253,
  "power": 276.4,
  "energy_kwh": 12.482,
  "cost": 93.61,
  "state": "NORMAL",
  "uptime_s": 4832
}
```

Fault messages are sent when the fault code or system state changes:

```json
{
  "fault": "OVERCURRENT",
  "state": "FAULT",
  "voltage": 228.1,
  "current": 13.1,
  "power": 2988.0,
  "timestamp_ms": 1234567
}
```

Diagnostics include firmware version, OTA state/counters, uptime, heap, Wi-Fi and MQTT reconnects, Modbus requests, fault count, offline buffer depth, and dropped telemetry.

OTA state changes use the separate `ota` topic and include the current and target versions. They are published on transitions rather than continuously.

## Presence and reconnect behavior

After connecting, the service publishes retained `online` on the status topic. The MQTT Last Will is retained `offline`. A broker can therefore show that a device disappeared even when the device could not publish a final message. This is useful for remote monitoring because stale telemetry and an active device are different conditions.

Reconnects are timestamp-based. There is no `while (!connected())` loop. Wi-Fi or MQTT failure increments diagnostics and does not block EnergyTask, FaultTask, or StorageTask.

## Offline buffer

Each `TelemetryRecord` contains:

- timestamp
- `EnergyData`
- `SystemState`
- `FaultCode`

The buffer is a fixed-capacity circular array. When full, it drops the oldest record and increments `telemetryDroppedCount`. This keeps recent readings, which are usually more useful for current operational decisions than stale history.

After reconnect, NetworkTask replays at most `mqttDrainPerLoop` records per iteration. A failed replay remains at the head of the buffer so it can be retried later.

## Limitations

Fault and diagnostics messages are best effort. Telemetry has bounded offline retention but is not a durable database. Broker authentication and TLS policy still require target deployment configuration and validation.
