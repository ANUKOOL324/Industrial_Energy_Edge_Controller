# MQTT Telemetry

## What is MQTT?

MQTT is a lightweight publish-and-subscribe messaging protocol. A device publishes a message to a topic, and a broker forwards that message to interested clients.

This project uses MQTT to send energy readings and device status to a broker. MQTT is only a communication path. Energy measurement does not depend on it.

## Where MQTT runs

`NetworkTask` owns `MqttService`. `EnergyTask` only produces `EnergyData`; it never calls MQTT. This separation means a broker outage cannot stop sensor sampling, fault evaluation, or NVS storage.

```text
EnergyTask -> Network queue -> NetworkTask -> MqttService -> MQTT broker
                                      |
                                      +-> Offline telemetry buffer
```

## Configuration

| Setting | Location | Purpose |
|---|---|---|
| Broker address | `config::mqttBroker` | MQTT server address |
| Broker port | `config::mqttPort` | MQTT server port |
| Device ID | `config::mqttDeviceId` | Topic namespace |
| Publish interval | `config::mqttPublishPeriodMs` | Live telemetry interval |
| Reconnect interval | `config::mqttReconnectIntervalMs` | Minimum retry interval |
| Buffer capacity | `config::telemetryBufferCapacity` | Maximum stored offline records |
| Username/password | local `include/secrets.h` | Optional broker credentials |

## Topics

With the default device ID, topics are:

```text
industrial-energy/meter01/telemetry
industrial-energy/meter01/status
industrial-energy/meter01/fault
industrial-energy/meter01/diagnostics
industrial-energy/meter01/ota
```

## Telemetry messages

Telemetry contains values already produced by the controller:

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

Fault messages are sent when the fault code or state changes. Diagnostics include firmware, heap, uptime, reconnect, Modbus, fault, buffer, and OTA information.

## Online and offline status

After a successful connection, the service publishes retained `online` status. It also configures a retained Last Will message containing `offline`.

The Last Will is useful because a device may lose power or network access without having time to publish a final message. The broker can then show that the device is no longer connected.

## Reconnection

The service records the last connection attempt and retries after `mqttReconnectIntervalMs`. It does not use a loop that waits until the broker connects.

```text
Wi-Fi unavailable -> wait -> retry Wi-Fi
Wi-Fi available, MQTT unavailable -> wait -> retry MQTT
MQTT connected -> publish live telemetry and replay buffered telemetry
```

## Offline telemetry buffer

When MQTT is unavailable, NetworkTask stores a `TelemetryRecord` in a fixed-size circular buffer. A record contains:

- timestamp
- `EnergyData`
- system state
- fault code

When the buffer is full, the oldest record is discarded. This keeps the newest measurements, which are more useful for current operation than stale readings.

After reconnect, only a limited number of records are sent per NetworkTask cycle. This avoids flooding the broker.

## Limitations

Fault and diagnostics messages are best-effort events. Telemetry has bounded offline retention; it is not a permanent database. Broker credentials and deployment security settings belong in the local configuration.
