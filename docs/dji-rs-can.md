# DJI R SDK: Protocol and User Interface

**Version:** V2.2  
**Date:** 2020.10  
**Company:** SZ DJI TECHNOLOGY CO., LTD.  
**Contact:** Ronin.SDK@dji.com

---

## Release Notes

| Version | Date | Section | Reason for Change | Description of Change |
| :--- | :--- | :--- | :--- | :--- |
| 1.0.0.0 | July 17, 2019 | | | Draft document |
| 2.0.0.0 | Oct 8, 2019 | 3 | Deleted sample code, added CRC params | 1. First release<br>2. Added CRC model parameters description |
| 2.1.0.1 | May 11, 2020 | 2.3, 3.3, 3.4 | Added commands and CRC pattern sample | 1. Added module version protocol<br>2. Added sample of command group pack<br>3. Added CRC sample code |
| 2.1.0.2 | June 17, 2020 | 2.3, 3.1 | Added external device control and hardware support | 1. Added joystick command<br>2. Added CAN support |
| 2.2.0.3 | June 22, 2020 | 2.3 | Added commands | 1. Obtain/set handheld gimbal user parameters<br>2. Set gimbal operating mode<br>3. Added Recenter and Selfie<br>4. Added third-party camera motion command |
| 2.2.0.4 | July 16, 2020 | 2.3 | Added commands | 1. Follow Mode settings<br>2. Auto Tune settings and info push<br>3. ActiveTrack settings<br>4. Obtain camera status |

---

## 1. Introduction
The DJI R SDK protocol is a simple, stable, and reliable communication protocol for third-party control of handheld gimbal movements and status retrieval.

## 2. Protocol Description

### 2.1 Data Format
| SOF | Ver/Length | CmdType | ENC | RES | SEQ | CRC-16 | DATA | CRC-32 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1-byte | 2-byte | 1-byte | 1-byte | 3-byte | 2-byte | 2-byte | n-byte | 4-byte |

### 2.2 Field Descriptions
- **SOF (0):** Header, fixed at **0xAA**.
- **Ver/Length (1):** 
  - `[15:10]`: Version number (default 0).
  - `[9:0]`: Total frame length (LSB first).
- **CmdType (3):**
  - `[4:0]`: Reply type (0: No reply, 1: Optional, 2-31: Required).
  - `[5]`: Frame type (0: Command frame, 1: Reply frame).
  - `[7:6]`: Reserved.
- **ENC (4):** 
  - `[4:0]`: Padding length for 16-byte alignment (encryption).
  - `[7:5]`: Encryption type (0: Unencrypted, 1: AES256).
- **RES (5):** Reserved (3 bytes).
- **SEQ (8):** Serial number (2 bytes).
- **CRC-16 (10):** Frame header check.
- **DATA (12):** Data segment (variable length `n`).
- **CRC-32 (n+12):** Entire frame check.

#### Data Segment (Command Frame)
| Domain | Offset | Size | Description |
| :--- | :--- | :--- | :--- |
| CmdSet | 0 | 1 | Command set |
| CmdID | 1 | 1 | Command code |
| CmdData | 2 | n-2 | Data content |

---

## 2.3 Detailed Descriptions

### 2.3.1 Commands Set and IDs (Partial List)

| CmdSet | CmdID | Description |
| :--- | :--- | :--- |
| **0x0E** | 0x00 | Control gimbal position |
| | 0x01 | Control gimbal speed |
| | 0x02 | Obtain angle information (Joint/Attitude) |
| | 0x03 | Set limit angle |
| | 0x05 | Set motor stiffness |
| | 0x08 | Parameter Push (Gimbal status) |
| | 0x09 | Obtain module version |
| | 0x0A | External Device (Joystick) Control |
| | 0x0B | Obtain User Parameters (TLV) |
| | 0x0C | Set User Parameters (TLV) |
| | 0x0D | Set Operating Mode (Landscape/Portrait) |
| | 0x0F | Auto Calibration Settings |
| | 0x11 | ActiveTrack Settings |
| **0x0D** | 0x00 | Third-Party Camera Motion |
| | 0x01 | Obtain Camera Status |

### 2.3.2 Return Codes
- **0x00:** Success
- **0x01:** Parse error
- **0x02:** Execution failure
- **0xFF:** Undefined error

### 2.3.3 Device IDs
- **0x00000001:** DJI R SDK
- **0x00000002:** Remote Controller

---

## 2.3.4 Gimbal Control Commands

### 2.3.4.1 Position Control (Set 0x0E, ID 0x00)
- **Data (8 bytes):**
  - `yaw_angle` (int16): 0.1° units (-1800 to +1800).
  - `roll_angle` (int16): 0.1° units (-1800 to +1800).
  - `pitch_angle` (int16): 0.1° units (-1800 to +1800).
  - `ctrl_byte` (uint8): 
    - `[3:1]`: Axis invalid (0: Valid, 1: Invalid) for Pitch, Roll, Yaw.
    - `[0]`: Mode (0: Incremental, 1: Absolute).
  - `time_for_action` (uint8): Execution speed in 0.1s units.

### 2.3.4.2 Speed Control (Set 0x0E, ID 0x01)
*Note: Valid for 0.5s only; send periodically.*
- **Data (7 bytes):**
  - `yaw/roll/pitch_speed` (int16): 0.1°/s units.
  - `ctrl_byte` (uint8): `[7]` Control Bit (1: Take over), `[3]` Focal length impact (0: On).

### 2.3.4.9 Parameter Push (Set 0x0E, ID 0x08)
Reports current state:
- `ctrl_byte`: `[0]` Angle info valid, `[1]` Limit info valid, `[2]` Stiffness info valid.
- Followed by angles (Yaw, Roll, Pitch, Joint angles), Limit angles, and Stiffness values.

### 2.3.4.11 External Control (Joystick/Dial)
- **Joystick:** `device_type=0x01`. Speeds for Pitch, Roll, Yaw (int16, -15000 to 15000).
- **Dial:** `device_type=0x02`. Speed (int16, -2048 to 2048).

### 2.3.4.14 Operating Mode (Set 0x0E, ID 0x0D)
- `mode=0xFE`: Unchanged.
- `landscape_portrait`: 0x01-0x02 Landscape, 0x03-0x04 Portrait, 0x05 Switch, 0xFF Default.

---

## 3. Hardware & Software Support

### 3.1 Hardware (CAN)
- **Baudrate:** 1M
- **Frame:** Standard
- **Tx ID:** 0x223 (to Gimbal)
- **Rx ID:** 0x222 (from Gimbal)

### 3.1.2 NATO/RSA Port Pinout
1. **VCC:** 8V ± 0.4V (0.8A - 1.2A)
2. **CANL**
3. **SBUS_RX**
4. **CANH**
5. **AD_COM:** Accessory detect (10-100k pull-down)
6. **GND**

### 3.2 Software (CRC)
- **CRC16:** Poly 0x8005, Init 0xC55C, RefIn/Out True.
- **CRC32:** Poly 0x04C11DB7, Init 0xC55C0000, RefIn/Out True.

---

## 3.3 Example Packet
**Gimbal Position Control:**
`AA 1A 00 03 00 00 00 00 22 11 A2 42 0E 00 20 00 30 00 40 00 01 14 7B 40 97 BE`
- Header: `AA 1A 00 03 00 00 00 00 22 11`
- CRC16: `A2 42`
- Data: `0E 00 20 00 30 00 40 00 01 14`
- CRC32: `7B 40 97 BE`
