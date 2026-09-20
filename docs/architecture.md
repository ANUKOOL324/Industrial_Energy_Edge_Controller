# Firmware architecture

```text
ADC sensors -> EmonLib -> EnergyMeter -> EnergySample
                                      |-> NVS checkpoint (EnergyStore)
                                      |-> Blynk V0..V4
                                      |-> future MQTT / Modbus / diagnostics
```

`energy_logic` contains platform-independent integration, tariff and cutoff logic. This makes the most failure-prone accounting behavior testable without an ESP32. `EnergyStore` owns NVS access and is the only persistence boundary. `main.cpp` owns scheduling and transport compatibility for the current Blynk deployment.

The original sketch had all of these responsibilities in one file, blocked during Wi-Fi startup, embedded credentials, and zeroed totals whenever the measured load was below a cutoff. Those behaviors are removed in this revision.

