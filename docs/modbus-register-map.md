# Modbus TCP

## What is Modbus TCP?

Modbus TCP is a simple request-and-response protocol commonly used by PLCs, HMIs, SCADA systems, and industrial software.

This project uses Modbus TCP to provide read-only energy values to another device on the network. The Modbus client can read registers, but it cannot change controller settings through this interface.

## How a request is handled

```text
Industrial client
      |
      v
Wi-Fi TCP connection on port 502
      |
      v
ModbusServer in NetworkTask
      |
      v
Scaled register response
```

The server accepts function code `0x03` for holding registers and `0x04` for input registers. It validates the Modbus TCP header, unit ID, function code, address, and register count. Unsupported or malformed requests receive an exception response.

## Register addressing

The implementation uses zero-based internal offsets. Some client tools display internal offset `0` as holding register `40001`.

| Internal offset | Common display | Name | Encoding | Unit | Access |
|---:|---:|---|---|---|---|
| 0 | 40001 | Voltage | value x 100 | V | Read |
| 1 | 40002 | Current | value x 1000 | A | Read |
| 2 | 40003 | Real power | value x 10 | W | Read |
| 3-4 | 40004-40005 | Energy | 32-bit high word, then low word | Wh | Read |
| 5 | 40006 | System state | `0 NORMAL`, `1 WARNING`, `2 FAULT`, `3 RECOVERY` | enum | Read |
| 6 | 40007 | Active fault | Numeric `FaultCode` | enum | Read |
| 7 | 40008 | Free heap | Unsigned integer | KiB | Read |
| 8-9 | 40009-40010 | Uptime | 32-bit high word, then low word | seconds | Read |
| 10-11 | 40011-40012 | Cost | 32-bit high word, then low word | cents | Read |

## Scaling examples

Modbus registers are 16-bit integers, so floating-point values are converted using documented scales:

```text
229.54 V -> 22954
1.253 A  -> 1253
276.4 W  -> 2764
```

Energy, uptime, and cost use two registers. The high word is sent first, followed by the low word.

## Current limits

The server is read-only. It does not implement register writes, Modbus RTU, RS-485, authentication, or access control. TCP and hardware testing are separate deployment activities.
