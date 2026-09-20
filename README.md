# Industrial Energy Edge Controller

ESP32 firmware for voltage, current, power, energy and tariff tracking using the existing EmonLib sensor path.

## Build and configure

1. Install PlatformIO.
2. Copy `include/secrets.example.h` to `include/secrets.h`.
3. Put the Wi-Fi, Blynk and MQTT values for the target installation in `include/secrets.h`.
4. Run `pio run -e esp32dev` and flash the board.

The local secrets file is ignored by Git. Credentials that were present in the original sketch were removed from the tracked source; rotate those Wi-Fi and Blynk credentials before deploying.

## Current firmware behavior

The controller keeps the original hardware mapping: voltage on GPIO 35, current on GPIO 34, and the original calibration constants. Measurements are filtered by configurable cutoffs, accumulated using elapsed time, and checkpointed to ESP32 NVS once per minute. A missing load no longer resets the accumulated energy or cost.

The firmware reconnects to Wi-Fi through the ESP32 station stack without a startup retry loop. Blynk telemetry remains available on V0-V4 for compatibility with the original dashboard.

Run the portable logic tests with `pio test -e native`.

## Planned expansion

The next implementation slice can add the industrial interfaces from the brief behind the same measurement model: MQTT with a bounded offline queue, Modbus TCP register publishing, FreeRTOS task separation, watchdog health reporting, and a simulation input mode. The current code keeps sensor acquisition, persistence and transport boundaries small so those additions do not require another monolithic sketch.
