# CISYNTH CONTACT IMAGE SYNTHESIZER MAX8 External

![Spectral Sound Scanner](https://reso-nance.org/wp-content/uploads/2023/06/20230709_135345-1140x624.jpg)

## Description

The CIS is a tangible interface for creating music and visuals: a contact image
sensor scanned at up to 1 kHz, an inertial measurement unit, three buttons with
backlight and a 256x64 OLED, all on a PoE Ethernet link. Since firmware 4.0 the
device talks to the Sp3ctra VST through **Sp3ctra Link (SLP)**, a small UDP
protocol with discovery, an exclusive host session, a negotiated image stream,
a buttons + IMU stream and host -> device feedback (LEDs, OLED overlay).

## Project Status

This project is now in an advanced stage of maturity, nearing the product’s commercialization. Please note that while most features are complete, minor adjustments may still be made as we prepare for the final release.

## Features

### Power over Ethernet (PoE)

Our device supports Power over Ethernet (PoE), which simplifies cabling and installation by allowing both electrical power and data transfer over a single Ethernet cable.

### Inertial Measurement Unit (IMU)

The integration of an inertial measurement unit enables precise 3D gesture tracking, allowing for detailed interaction.

### Sp3ctra Link (SLP v1)

The wire contract is `Common/Inc/sp3ctra_link.h` (copied byte-for-byte into the
VST). Two UDP flows:

| Flow | Port | Direction | Content |
|---|---|---|---|
| CONTROL | **55150** (device listens) | host <-> device | `HELLO` -> `ANNOUNCE` (identity + capabilities), `BIND` -> `BIND_ACK` (negotiated stream layout), `PING`/`PONG` every 500 ms, `LED_SET`, `OLED_OVERLAY`/`OLED_CLEAR`, `CFG_GET`/`CFG_SET`/`CFG_REPLY`, `CAL_START`, `ERROR` |
| STREAM | **55151** (chosen by the host in `BIND`) | device -> host | `LINE` (one datagram per 288-pixel fragment, 12 fragments at 400 DPI / 6 at 200 DPI) and `HID` (buttons + accelerometer in g + gyroscope in dps + temperature, 200 Hz by default and immediately on every button edge) |

- Discovery: the host broadcasts `HELLO` on the subnet; every device answers
  `ANNOUNCE` with its unique name (`Sp3ctra-XXXX`), serial, MAC, firmware and
  capabilities. No mDNS, no static host address needed.
- Session: one host at a time (`BIND` from another peer is answered `BUSY`).
  The stream goes to the address the `BIND` came from (or a multicast group).
  Without `PING` for 3 s the session expires and the device falls back to the
  static *Dest IP / Stream Port* configured on the web page ("Stream w/o host",
  can be turned off).
- Identity: the MAC (`02:53:33:xx:xx:xx`), name and serial derive from the MCU
  unique id (`Common/Src/sys_identity.c`).

Test without the VST: `scripts/slp/slp_tool.py discover | stat | hid | lines |
led | overlay | cfg | cal` (see `scripts/README.md`). A simulated device for the
VST side lives in `scripts/slp/slp_fake_device.py`.

### HTTP Server

The device also runs an HTTP server for configuration from a browser and
firmware upload. Navigate to the device IP address (default:
[192.168.100.1](http://192.168.100.1/config.html)). Sections:

#### Device

Name, serial number, MAC address and host link state (bound / streaming), refreshed every second.

#### CIS Parameters

- **DPI (Dots Per Inch)**:  
  Configures the resolution of the Contact Image Sensor (CIS). Available options:
  - 200 DPI
  - 400 DPI

- **OVSP (Oversampling)**:  
  Adjusts the oversampling rate to enhance image quality.

- **LPS (Lines Per Second)**:  
  Visualizes the line capture rate. Higher values improve performance but may reduce image quality.

- **Hand (Left/Right)**:  
  Select the dominant hand for accurate calibration.

- **Start Calibration**:  
  Initiates the calibration process based on the selected settings.

#### Network Settings

- **IP Address/Subnet Mask/Gateway**: static IPv4 configuration of the device
  (default `192.168.100.1` / `255.255.255.0` / `0.0.0.0`).
- **Dest IP Address / Stream Port**: where `LINE`/`HID` datagrams go while no
  host session is bound (default `192.168.100.10:55151`).
- **Link Port (SLP)**: control channel port the device listens on (default `55150`).
- **Stream w/o host**: keep streaming to *Dest IP* without a host session (ON by default).

> **Note**: After modifying network settings, click **Apply Network Settings** (the device reboots).

#### Firmware Update

To update the firmware via the HTTP interface:

1. Select the firmware file from your local machine.
2. Click **Upload Firmware** to initiate the update process.

#### Advanced Settings

- **Factory Reset**:  
  Restores the device to its original factory settings. Use this option to reset all configurations if needed.

### FTP Server

The CISYNTH device is also equipped with an FTP server, allowing file transfers to and from the device. This server can be accessed using any FTP client.

#### Connection Parameters

- **Protocol**:  
  Be aware that this connection is not encrypted, so avoid transmitting sensitive data.

- **Host**:  
  The IP address of the device should be entered here. By default, this is:
  - IP: `192.168.0.10`

- **Port**:  
  You can use the default FTP port (`21`) unless you have configured the server to use a custom port.

- **Encryption**:  
  The FTP connection used is non-encrypted (FTP simple).

- **Authentication Type**:  
  - **Anonymous**:  
    By default, the server allows anonymous access, meaning no username or password is required to connect to the FTP server.  
    You can adjust this setting for added security if needed.

#### Using an FTP Client

1. Open your preferred FTP client (e.g., FileZilla).
2. Enter the connection details as described above (IP address, protocol, etc.).
3. Connect to the server. For anonymous access, leave the username and password fields blank.
4. Once connected, you can upload or download files to and from the device.

## Using MAX8

Download our Max examples along with the **cis_receive** external from [Max Patchs](https://github.com/Ondulab/CISYNTH_Max_Patchs) and connect the CISYNTH.

Manually configure the network connection:

- **IP Address**: `192.168.0.1`
- **Subnet Mask**: `255.255.255.0`
- **Gateway**: `0.0.0.0`

## Contributions

Contributions to this project are welcome. Please submit your pull requests or report issues via GitHub.  
For more information on the Spectral Sound Scanner and other innovative projects, visit our website at [Réso-nance Numérique](https://reso-nance.org/).

Sources :
[Firmware](https://github.com/Ondulab/Sp3ctra_CIS_Firmware),
[Bootloader](https://github.com/Ondulab/CISYNTH_CIS_Bootloader),
[Electronics](https://github.com/Ondulab/CISYNTH_CIS_Electronics), 
[Mechanics](https://github.com/Ondulab/CISYNTH_CIS_Mechanics), 
[Max Patchs](https://github.com/Ondulab/CISYNTH_Max_External).

## Technical Specifications

| **Characteristic**          | **Details**                                 |
|-----------------------------|---------------------------------------------|
| **Weight**                  | 290g                                        |
| **Dimensions**              | L 264mm x W 32mm x H 21mm                   |
| **Connector**               | RJ45 Ethernet                               |
| **Power Supply**            | 12V PoE                                     |
| **Max Power Consumption**   | 10W                                         |
| **Display**                 | OLED screen 256x64                          |
| **Buttons**                 | 3 physical buttons                          |
| **Image Sensor**            | M118-232C3_V1.51                            |
| **Inertial Measurement Unit**| ICM42688                                    |
| **Operating Temperature Range** | 0°C to 40°C                              |
| **Compliance**              | CE Marking                                  |

## License

Copyright (C) 2018-present Réso-nance Numérique

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

## Credits and Acknowledgements

We would like to extend our sincere thanks to DEVISUBOX for their support and contributions to this project. Their assistance has been invaluable in our development process.

## Contact 

For any questions or inquiries, you can also contact us via email at **contact@reso-nance.org**.
For an opportunity to test our products, please reach out to us.
