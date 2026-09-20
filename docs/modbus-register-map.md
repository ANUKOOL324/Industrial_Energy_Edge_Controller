# Modbus TCP Register Map

The controller contains a small read-only Modbus TCP server built on the ESP32 `WiFiServer` API. It listens on TCP port `502` by default and is processed by `NetworkTask`.

## Supported requests

- Function `0x03`: Read Holding Registers
- Function `0x04`: Read Input Registers
- Writes are not implemented.

The server validates the transaction ID, protocol ID, MBAP length, unit ID, function code, start address, and requested register count. Malformed or unsupported requests receive an exception response instead of being interpreted as data.

## Addressing

The implementation uses zero-based internal offsets. Some client tools display holding register offset `0` as user-facing register `40001`.

| Internal offset | Common display | Name | Encoding | Unit | Access |
|---:|---:|---|---|---|---|
| 0 | 40001 | Voltage | unsigned integer, volts x 100 | V | Read |
| 1 | 40002 | Current | unsigned integer, amps x 1000 | A | Read |
| 2 | 40003 | Real power | unsigned integer, watts x 10 | W | Read |
| 3-4 | 40004-40005 | Energy | unsigned 32-bit, high word then low word | Wh | Read |
| 5 | 40006 | System state | `0 NORMAL`, `1 WARNING`, `2 FAULT`, `3 RECOVERY` | enum | Read |
| 6 | 40007 | Active fault | Numeric `FaultCode` | enum | Read |
| 7 | 40008 | Free heap | Unsigned integer | KiB | Read |
| 8-9 | 40009-40010 | Uptime | Unsigned 32-bit, high word then low word | seconds | Read |
| 10-11 | 40011-40012 | Cost | Unsigned 32-bit, high word then low word | cents | Read |

## Encoding examples

```text
229.54 V  -> 22954
1.253 A   -> 1253
276.4 W   -> 2764
```

Values are encoded explicitly as integers. Negative values, NaN, infinity, and values above the 16-bit range are clamped by the portable codec. Larger values use two 16-bit words in network order.

## Limitations

This is an educational telemetry server, not a complete Modbus implementation. It does not provide register writes, authentication, access control, or Modbus RTU/RS-485 support. TCP and hardware validation remain pending.
