# Serial Protocol - ESP32-Can to SBC Interface

## Overview

The M5Stack Atom (ESP32) connects to the Khadas VIM4 (SBC) via USB cable. The SBC sends simple serial commands that the ESP32 converts to DJI CAN bus protocol.

## Connection

```
SBC (VIM4) <-- USB Cable --> AtomS3 <-- CAN Bus --> DJI Ronin RS4
```

- USB provides power + serial communication
- Serial: 115200 baud, 8N1

## Command Format

```
CAN:CC:II:DD...\n
```

| Field | Description | Example |
|-------|-------------|---------|
| `CAN:` | Fixed header | `CAN:` |
| `CC` | Command Set (hex, 1 byte) | `0E` |
| `II` | Command ID (hex, 1 byte) | `00` |
| `DD...` | Data bytes (hex, up to 16 bytes) | `0000000000000080` |
| `\n` | Line terminator | `\n` |

**Example:** Position control (0x0E:0x00)
```
CAN:0E:00:0000000000000080
```

## Quick Reference Commands

### Gimbal Control (CmdSet 0x0E)

| CmdID | Name | Data | Description |
|-------|------|------|-------------|
| `00` | Position | yaw, roll, pitch, ctrl, time | Set absolute position (0.1° units) |
| `01` | Speed | yaw, roll, pitch, ctrl | Control speed (0.1°/s, valid 0.5s) |
| `02` | Get Angle | - | Request angle info |
| `08` | Status | - | Request gimbal status |
| `09` | Version | - | Get firmware version |
| `0D` | Mode | mode | Landscape/Portrait mode |
| `10` | Recenter | 00 | Recenter gimbal |

### Position Control Data (CmdID 0x00) - 8 bytes
```
yaw(2bytes) roll(2bytes) pitch(2bytes) ctrl(1byte) time(1byte)
```
- Angles: int16, 0.1° units, range -1800 to +1800
- ctrl: bit0=mode(0=inc,1=abs), bits1-3=axis valid
- time: action duration in 0.1s

### Speed Control Data (CmdID 0x01) - 7 bytes
```
yaw(2bytes) roll(2bytes) pitch(2bytes) ctrl(1byte)
```
- Speeds: int16, 0.1°/s units
- ctrl: bit7=takeover, bit3=focal length impact

### Examples

**Recenter gimbal:**
```
CAN:0E:10:00
```

**Set pan (yaw) speed 40°/s right:**
```
CAN:0E:01:00FA000000000080
```
yaw = 400 = 40.0°/s

**Set pan 40°/s left:**
```
CAN:0E:01:00FA000000000000
```
yaw = -400 (0xFE06) = -40.0°/s

**Request status:**
```
CAN:0E:08:
```

**Get version:**
```
CAN:0E:09:
```

## Python Example (SBC Side)

```python
import serial

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

def send_gimbal_cmd(cmd_set, cmd_id, data_hex=""):
    cmd = f"CAN:{cmd_set:02X}:{cmd_id:02X}:{data_hex}\n"
    ser.write(cmd.encode())
    print(f"Sent: {cmd.strip()}")

# Recenter
send_gimbal_cmd(0x0E, 0x10, "00")

# Pan right at 40°/s
send_gimbal_cmd(0x0E, 0x01, "00FA000000000080")

# Pan left at 40°/s
send_gimbal_cmd(0x0E, 0x01, "00FA000000000000")

# Set absolute position (yaw=0°, pitch=0°, roll=0°)
send_gimbal_cmd(0x0E, 0x00, "0000000000000080")
```

## Status Responses

The ESP32 echoes commands but does not forward gimbal responses back over serial (CAN Rx messages are displayed on the M5Stack screen only).
