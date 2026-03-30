# DJI RS4 Gimbal CAN Controller

A high-performance ESP32-based CAN bus controller for the DJI RS4 / RS4 Pro gimbal series. This application provides real-time pan/tilt control and status monitoring across multiple M5Stack hardware platforms.

## 🚀 Features

*   **Multi-Board Support:** Unified codebase for M5Stack Core2, Atom S3, and Stamp S3.
*   **DJI RS SDK v2.2 Implementation:** Fully verified CRC16/CRC32 and packet framing logic.
*   **Dual-Protocol Ready:** Designed for the 0xAA SDK protocol with support for 0x55 internal bus monitoring.
*   **Real-time UI:** 
    *   **Core2:** Interactive touch D-pad for continuous speed control.
    *   **Atom S3:** Physical screen-button for instant gimbal re-centering.
*   **Smart Bus Monitoring:** Visual status bar (Green/Red) indicating live CAN traffic health.
*   **Automatic Fragmentation:** Handles DJI packets larger than the standard 8-byte CAN limit.

---

## 🏗️ Architecture

The project is built on **PlatformIO** using the **pioarduino** (Community) ecosystem to support the latest ESP32 v3.0+ Arduino cores.

### 1. Hardware Abstraction Layer
The application uses `M5Unified` for device detection and display handling, combined with the native ESP-IDF `twai` (Two-Wire Automotive Interface) driver for low-level CAN communication.

### 2. Protocol Engine
The controller implements the specific DJI RS SDK requirements:
*   **Baud Rate:** 1 Mbps
*   **Endianness:** 
    *   Length: Little Endian (LSB First)
    *   Sequence Number: **Big Endian** (MSB First)
    *   CRCs: Little Endian
*   **Checksums:** Custom implementation of DJI's Reflected CRC16 and CRC32 using the `0xC55C` initialization constant.

### 3. Pin Mapping
| Board | CAN TX | CAN RX | User Button |
| :--- | :--- | :--- | :--- |
| **M5Stack Core2** | G32 (Port A) | G33 (Port A) | Touch Screen |
| **M5Stack Atom S3** | G2 (Port A) | G1 (Port A) | G41 (Screen) |
| **M5Stack Stamp S3** | G13 | G15 | G0 |

---

## 🛠️ Build & Installation

### Prerequisites
*   [PlatformIO IDE](https://platformio.org/)
*   **pioarduino** extension (Community maintained PlatformIO for ESP32 v3.0+)

### Building for specific boards
Use the Environment Switcher in PlatformIO or the CLI:

```bash
# Build & Upload for Core2
pio run -e m5stack-core2 -t upload

# Build & Upload for Atom S3
pio run -e m5stack-atoms3 -t upload
```

---

## 🎮 UI Guide

### Core 2 (Touch)
*   **UP/DN/L/R Arrows:** Hold to rotate the gimbal at 40°/s. Release to stop instantly.
*   **Red Center Circle:** Recenter all axes to 0°.
*   **Top Bar:** Displays "CAN OK" (Green) or "NO BUS" (Red).

### Atom S3 (Physical)
*   **Push Screen:** Triggers a full gimbal re-center.
*   **Top Bar:** Real-time green/red connection status.

---

## 📝 Configuration (`platformio.ini`)
The project uses `build_flags` to manage board-specific GPIOs. If using a custom transceiver, update the `CAN_TX` and `CAN_RX` definitions in the environment section of `platformio.ini`.

```ini
[env:m5stack-atoms3]
build_flags = 
    -D BOARD_ATOMS3
    -D CAN_TX=2
    -D CAN_RX=1
```

---

## 🤝 Acknowledgments
Protocol logic and CRC verification assisted by the **4D Project** and official **DJI RS SDK V2.2** documentation.
