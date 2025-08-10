# 🌱 WaterNode

**WaterNode** is an automatic drip irrigation system based on CH32V003 microcontroller nodes and a Raspberry Pi central controller.  
Nodes communicate over **RS-485**, control valves, and read soil moisture sensors. The Raspberry Pi runs a **web dashboard** for monitoring and control.

---



## Features
- Automatic node addressing over RS-485
- Valve control (manual and automatic)
- Soil moisture monitoring
- Web dashboard with live charts
- Event logging
- Sensor calibration

---
## Structure
```
WaterNode/
  ├─ ch32v003_firmware/   # Firmware for CH32V003 nodes
  └─ rpi/                 # Python application for Raspberry Pi
```

---

## Communication Protocol

**Transport:** RS-485 (half-duplex)  
**Baud rate:** configurable in firmware (`UART_BAUDRATE`)

**Packet format:**
```
[ADDR][CMD][LEN][DATA...][CRC_L][CRC_H]
```
- **ADDR** — device address (`0x00` for broadcast)
- **CMD** — command code
- **LEN** — payload length in bytes
- **DATA** — command-specific data
- **CRC_L / CRC_H** — CRC16-IBM checksum (poly `0xA001`, init `0xFFFF`, little-endian)

---

### Command list (current implementation)

| Example (hex)                             | CMD   | Name                    | Direction       | Description |
|-------------------------------------------|-------|------------------------|-----------------|-------------|
| `00 F0 00 35 C0`                          | 0xF0  | Get UID                | Master ↔ Node   | Request unique device ID from unaddressed node |
| `00 F1 05 DD FD 47 21 01 14 AB`           | 0xF1  | Set Address            | Master ↔ Node   | Assign address `0x01` to node with UID `DD FD 47 21` |
| `00 F2 04 DD FD 47 21 A7 A6`              | 0xF2  | Confirm Address        | Master ↔ Node   | Confirm address assignment for node with given UID |
| `01 01 00 21 90`                          | 0x01  | Ping                   | Master ↔ Node   | Check if node at address `0x01` is alive |
| `01 02 00 21 60`                          | 0x02  | Get Moisture           | Master ↔ Node   | Request soil moisture value |
| `01 03 01 00 F0 48`                       | 0x03  | Set Valve State        | Master ↔ Node   | Close valve (payload `0x00`) |
| `01 03 01 01 31 88`                       | 0x03  | Set Valve State        | Master ↔ Node   | Open valve (payload `0x01`) |
| `01 F3 02 01 A5 4A AF`                    | 0xF3  | Reset Address          | Master ↔ Node   | Request node to reset its assigned address |
| `01 F4 02 01 A5 4B DB`                    | 0xF4  | Confirm Address Reset  | Master ↔ Node   | Confirm address reset for node |

**CRC calculation:** CRC16-IBM (poly `0xA001`, init `0xFFFF`)

---

## Quick Start

**Clone the repository:**
```bash
git clone https://github.com/kr3nd31/WaterNode.git
cd WaterNode
```

**Build firmware:**
```bash
cd ch32v003_firmware
# Build with your toolchain (WCH IDE / PlatformIO / CMake)
```

**Run Raspberry Pi controller:**
```bash
cd rpi
pip install -r requirements.txt
python main.py
```

---

## License
MIT License