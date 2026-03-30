# DJI R SDK Protocol and User Interface
**Version:** V2.2
**Date:** 2020.10

## 1. DJI R SDK Protocol Introduction
The DJI R SDK protocol is a simple, easy, stable, and reliable communication protocol. A third party can control the handheld gimbal device movement and obtain its partial information via the DJI R SDK protocol. With the support of the DJI R SDK protocol, the handheld gimbal device has greater extensibility and can be applied in more scenarios.

## 2. DJI R SDK Protocol Description

### 2.1 Data Format
The data packet format of the DJI R SDK protocol is shown below:

| SOF | Ver/Length | CmdType | ENC | RES | SEQ | CRC-16 | DATA | CRC-32 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1-byte | 2-byte | 1-byte | 1-byte | 3-byte | 2-byte | 2-byte | n-byte | 4-byte |

*Figure 1: Data Packet Format*

### 2.2 Field Description

| Domain | Offset | Size | Descriptions |
| :--- | :--- | :--- | :--- |
| SOF | 0 | 1 | The frame header is set as 0xAA |
| Ver/Length | 1 | 2 | [15:10] - Version number (0 by default)<br>[9:0] - The length of the entire frame<br>Note: LSB first |
| CmdType | 3 | 1 | [4:0] - Reply type:<br>0: No reply is required after data is sent<br>1: Can reply or not after data is sent<br>2-31: Reply is required after data is sent<br>[5] - Frame type:<br>0: Command frame<br>1: Reply frame<br>[7:6] - Reserve (0 by default) |
| ENC | 4 | 1 | [4:0] - The length of supplementary bytes when encrypting (16-byte alignment is required when encrypting)<br>[7:5] - Encryption type:<br>0: Unencrypted<br>1: AES256 encryption |
| RES | 5 | 3 | Reserved byte segment |
| SEQ | 8 | 2 | Serial number |
| CRC-16 | 10 | 2 | Frame header check |
| DATA | 12 | n | Data segment (description shown below) |
| CRC-32 | n+12 | 4 | Frame check (the entire frame) |

*Figure 2: Data Packet Field Description*

---

### 2.3 Detailed Descriptions

#### 2.3.1 Commands Set and Command ID
There are two kinds of data segment content according to the frame type:

**1. Command Frame Data Segment Content:**
| Domain | Offset | Domain | Descriptions |
| :--- | :--- | :--- | :--- |
| CmdSet | 0 | 1 | Command set |
| CmdID | 1 | 1 | Command code |
| CmdData | 2 | n-2 | Data content |

**2. Reply Frame Data Segment Content:**
| Domain | Offset | Size | Descriptions |
| :--- | :--- | :--- | :--- |
| DATA | 0 | n | Data content |

**Command Sets and IDs:**
| CmdSet | CmdID | Descriptions |
| :--- | :--- | :--- |
| **0x0E** | 0x00 | Control handheld gimbal position (2.3.4.1) |
| | 0x01 | Control handheld gimbal speed (2.3.4.2) |
| | 0x02 | Obtain angle information (joint and attitude) (2.3.4.3) |
| | 0x03 | Set handheld gimbal limit angle (2.3.4.4) |
| | 0x04 | Obtain handheld gimbal limit angle (2.3.4.5) |
| | 0x05 | Set handheld gimbal motor stiffness (2.3.4.6) |
| | 0x06 | Obtain handheld gimbal motor stiffness (2.3.4.7) |
| | 0x07 | Set information push of gimbal parameters (2.3.4.8) |
| | 0x08 | Push handheld gimbal parameters (2.3.4.9) |
| | 0x09 | Obtain module version number (2.3.4.10) |
| | 0x0A | Push joystick control command (2.3.4.11) |
| | 0x0B | Obtain handheld gimbal user parameters (2.3.4.12) |
| | 0x0C | Set handheld gimbal user parameters (2.3.4.13) |
| | 0x0D | Set handheld gimbal operating mode (2.3.4.14) |
| | 0x0E | Set Recenter, Selfie, and Follow modes (2.3.4.15) |
| | 0x0F | Gimbal Auto Calibration Settings (2.3.4.16) |
| | 0x10 | Gimbal Auto Calibration Status Push (2.3.4.17) |
| | 0x11 | Gimbal ActiveTrack Settings (2.3.4.18) |
| **0x0D** | 0x00 | Third-party camera motion command (2.3.5.1) |
| | 0x01 | Third-party camera status obtain command (2.3.5.2) |

#### 2.3.2 Return Code
| Error Code Value | Implication |
| :--- | :--- |
| 0x00 | Command execution succeeds |
| 0x01 | Command parse error |
| 0x02 | Command execution fails |
| 0xFF | Undefined error |

#### 2.3.3 Device ID
Used to differentiate devices connecting to the DJI R SDK system.
| Device ID | Descriptions |
| :--- | :--- |
| 0x00000000 | Reserved |
| 0x00000001 | DJI R SDK |
| 0x00000002 | Remote controller |

---

### 2.3.4 Gimbal Command Set Data Segment Details (CmdSet = 0x0E)

#### 2.3.4.1 Handheld Gimbal Position Control (CmdID = 0x00)
| Frame Type | Offset | Size | Name | Type | Descriptions |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Command** | 0 | 2 | yaw_angle | int16_t | Unit: 0.1° (range: -1800 to +1800) |
| | 2 | 2 | roll_angle | int16_t | Unit: 0.1° (range: -1800 to +1800) |
| | 4 | 2 | pitch_angle | int16_t | Unit: 0.1° (range: -1800 to +1800) |
| | 6 | 1 | ctrl_byte | uint8_t | [7:4] Reserved (must be 0)<br>[3] Pitch axis valid: 0: Valid, 1: Invalid<br>[2] Roll axis valid: 0: Valid, 1: Invalid<br>[1] Yaw axis valid: 0: Valid, 1: Invalid<br>[0] Control mode: 0: Incremental, 1: Absolute |
| | 7 | 1 | time_for_action | uint8_t | Execution speed, unit: 0.1s. (e.g., 20 = 2s) |
| **Reply** | 0 | 1 | return code | uint8_t | Refer to 2.3.2 |

#### 2.3.4.2 Handheld Gimbal Speed Control (CmdID = 0x01)
*Note: Only controls for 0.5s per command for safety.*
| Frame Type | Offset | Size | Name | Type | Descriptions |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Command** | 0 | 2 | yaw_speed | int16_t | Unit: 0.1°/s (range: 0 to 3600) |
| | 2 | 2 | roll_speed | int16_t | Unit: 0.1°/s (range: 0 to 3600) |
| | 4 | 2 | pitch_speed | int16_t | Unit: 0.1°/s (range: 0 to 3600) |
| | 6 | 1 | ctrl_byte | uint8_t | [7] Control Bit: 0: Release speed control, 1: Take over<br>[6:4] Reserved<br>[3] Camera focal length impact: 0: Yes, 1: No<br>[2:0] Reserved |
| **Reply** | 0 | 1 | return code | uint8_t | Refer to 2.3.2 |

#### 2.3.4.3 Handheld Gimbal Information Obtaining (CmdID = 0x02)
| Frame Type | Offset | Size | Name | Type | Descriptions |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Command** | 0 | 1 | ctrl_byte | uint8_t | 0x00: No operation<br>0x01: Obtain attitude angle<br>0x02: Obtain joint angle |
| **Reply** | 0 | 1 | return code | uint8_t | Refer to 2.3.2 |
| | 1 | 1 | data_type | uint8_t | 0x00: Not ready, 0x01: Attitude, 0x02: Joint |
| | 2 | 2 | yaw | int16_t | Unit: 0.1° |
| | 4 | 2 | roll | int16_t | Unit: 0.1° |
| | 6 | 2 | pitch | int16_t | Unit: 0.1° |

#### 2.3.4.4 Handheld Gimbal Limit Angle Settings (CmdID = 0x03)
| Frame Type | Offset | Size | Name | Type | Descriptions |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Command** | 0 | 1 | ctrl_byte | uint8_t | 0x00: No op, 0x01: Set limit angle |
| | 1-6 | 1x6 | limits | uint8_t | pitch_max, pitch_min, yaw_max, yaw_min, roll_max, roll_min (Range: 0 to 179) |
| **Reply** | 0 | 1 | return code | uint8_t | Refer to 2.3.2 |

#### 2.3.4.5 Obtain Handheld Gimbal Limit Angle (CmdID = 0x04)
*Command Frame:* 0x01 to obtain.
*Reply Frame:* return code (1 byte) followed by the 6 limit values (pitch_max, pitch_min, yaw_max, yaw_min, roll_max, roll_min).

#### 2.3.4.6/7 Handheld Gimbal Motor Stiffness Settings/Obtain (CmdID = 0x05 / 0x06)
*Command Frame:* ctrl_byte (0x01 to set/obtain) + pitch, roll, yaw stiffness (uint8_t, 0-100).

#### 2.3.4.8/9 Handheld Gimbal Parameter Push Settings/Push (CmdID = 0x07 / 0x08)
| Frame Type | Offset | Size | Name | Type | Descriptions |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Push Data**| 0 | 1 | ctrl_byte | uint8_t | [0] Angle info valid: 0: Invalid, 1: Valid<br>[1] Angle limit info valid: 0: Invalid, 1: Valid<br>[2] Motor stiffness info valid: 0: Invalid, 1: Valid |
| | 1-12 | 2x6 | angles | int16_t | yaw_angle, roll_angle, pitch_angle, yaw_joint, roll_joint, pitch_joint (0.1°) |
| | 13-18 | 1x6 | limits | uint8_t | pitch_max/min, yaw_max/min, roll_max/min |
| | 19-21 | 1x3 | stiffness| uint8_t | pitch, yaw, roll stiffness (0-100) |

#### 2.3.4.10 Obtain Module Version Number (CmdID = 0x09)
*Reply:* 1-byte return code, 4-byte Device ID, 4-byte Version Number (0xAABBCCDD = AA.BB.CC.DD).

#### 2.3.4.11 External Device Control Command Push (CmdID = 0x0A)
*Controllers:* 0x00: Unknown, 0x01: Joystick, 0x02: Dial.
*Joystick Mapping:* Default maps Y/X to pitch/yaw.
*Data Segment (Joystick):* device_type (0x01), pitch_speed, roll_speed, yaw_speed (int16_t, -15000 to 15000).
*Data Segment (Dial):* device_type (0x02), dial_speed (int16_t, -2048 to 2048).

#### 2.3.4.12/13 Handheld Gimbal User Parameters (CmdID = 0x0B / 0x0C)
Uses TLV (ID + LENGTH + VALUE) format.
| ID | Name | Type | Length | Descriptions |
| :--- | :--- | :--- | :--- | :--- |
| 0x00 | Parameter Table | uint8_t | 1 | 0x00-0x02: Table 0-2 |
| 0x22 | Special Functions | uint8_t | 1 | [3-5] Roll 360 mode: 0: Normal, 1: 2-axis, 2: Roll 360, 3: 3D Roll 360 |
| 0x23 | Motor Special | uint8_t | 1 | [0] Power off motor |

#### 2.3.4.14 Operating Mode Settings (CmdID = 0x0D)
*Operating Mode:* 0xFE: Unchanged.
*Landscape/Portrait:* 0x00: No switch, 0x01: Lnd (0°), 0x02: Lnd (180°), 0x03: Prt (90°), 0x04: Prt (-90°), 0x05: Auto.

#### 2.3.4.15 Recenter, Selfie, and Follow Modes (CmdID = 0x0E)
| Mode | Value |
| :--- | :--- |
| Recenter | 0x01 |
| Selfie | 0x02 |
| Gimbal Lock | 0x00 (in Figure 28) |
| Yaw Follow | 0x02 (in Figure 28) |
| Sport Mode | 0x03 (in Figure 28) |

#### 2.3.4.16/17 Gimbal Auto Calibration (CmdID = 0x0F / 0x10)
Uses TLV. ID 0x00: bit [0] enable (0: stop, 1: start), bits [7:1] type (0: default, 1: single attitude).
*Status Push (0x10):* 6 bytes. Byte 0: status (0: none, 1: running, 2: complete, 3: error). Byte 1: progress (0-100).

#### 2.3.5 Camera Command Set (CmdSet = 0x0D)
**2.3.5.1 Motion Command (CmdID = 0x00):**
*Camera control command (Uint16_t):* 0x0001: shutter, 0x0002: stop shuttering, 0x0003: start record, 0x0004: stop record, 0x0005: center focus, 0x000B: end center focus.

**2.3.5.2 Status Obtain (CmdID = 0x01):**
*Reply Camera Status:* 0x00: not recording, 0x02: recording.

---

## 3. Notices

### 3.1 Hardware Support
The communication interface for DJI RS 2 is CAN.

| Baud rate | Frame type | CAN Tx | CAN Rx |
| :--- | :--- | :--- | :--- |
| 1M | Standard frame | 0x223 | 0x222 |

*Figure 34: CAN Communication Parameters*

#### 3.1.2 RSA/NATO Ports Pinout
| Pin | Signal | Description | Notes |
| :--- | :--- | :--- | :--- |
| 1 | VCC | Power output | 8V ± 0.4V, 0.8A (1.2A peak) |
| 2 | CANL | CANL | |
| 3 | SBUS_RX | SBUS input | |
| 4 | CANH | CANH | |
| 5 | AD_COM | Accessory detect | 10-100k pull down resistor. |
| 6 | GND | GND | |

### 3.2 Software Support
CRC parameters for data packets:

| Name | Width | Poly | Init | RefIn | RefOut | XorOut |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| CRC16 | 16 | 0x8005 | 0xc55c | True | True | 0x0000 |
| CRC32 | 32 | 0x04c11db7 | 0xc55c0000 | True | True | 0x00000000 |

### 3.3 Command Sample
Gimbal position control command example (Hex):
`AA 1A 00 03 00 00 00 00 22 11 A2 42 0E 00 20 00 30 00 40 00 01 14 7B 40 97 BE`
- **A2 42**: CRC-16
- **0E 00**: CmdSet (0x0E) and CmdID (0x00)
- **7B 40 97 BE**: CRC-32

### 3.4 CRC Code Sample
CRC16 and CRC32 can be verified using a utility with the following parameters:
- **CRC16:** poly=0x8005, init=0xc55c, reflect=true
- **CRC32:** poly=0x04c11db7, init=0xc55c0000, reflect=true
