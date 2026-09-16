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

The device also runs an HTTP server: pages, stylesheet and script all come from
its own flash, so any browser on the network drives it with nothing to install.
Navigate to the device IP address (default:
[192.168.100.1](http://192.168.100.1/), which opens the live image). Five tabs,
in the VST's colours and type (`vst/source/UITheme.h` -- category colour for what
you look at, one acid lime for what you touch):

| Tab | Page | What it is |
|---|---|---|
| SCAN | `/scan.html` | the sensor's image, live -- **and the CIS settings** (DPI, oversampling, handedness, calibration) |
| IMU | `/imu.html` | accelerometer, gyroscope and buttons, live -- **and the IMU settings** (full scales, calibration) |
| NETWORK | `/network.html` | device identity, host link, addresses and ports |
| GUI | `/gui.html` | what the OLED shows, screensaver and motion thresholds |
| UPDATE | `/update.html` | installed firmware, upload, network flash mode, device log, factory reset |

Each live view carries the settings that shape it: the DPI you pick on SCAN
changes the image right above it, and the full scales picked on IMU are the ones
its charts are drawn against. `fs/settings.js` holds what those pages share --
the reconnect modal, the numeric-input listeners, and the helpers that wait for
the device to answer again after a reboot.

The shell (logo header, tab bar, device identity) is built once by
`fs/sp3ctra.js`, and `fs/sp3ctra.css` holds the whole charter: a page adds a tab
by declaring `<body data-page="...">`, never by copying markup. Nothing is
fetched from the internet -- an isolated LAN is the normal case.

Every page is **one module**, laid out like a module editor in the VST
(`ui/ModuleEditorChrome.h`): the frame with its caption and readout, a status
strip under it, then rows of boxes -- each control under a small centred label --
split by `--- SECTION ---` bands. Mutually exclusive choices (200/400,
LEFT/RIGHT, OFF/ON) are drawn as one segmented control, not as loose buttons.
Stacking panels of equal weight is what this replaced.

#### SCAN

[`/scan.html`](http://192.168.100.1/scan.html) shows what the sensor sees. The
page polls `GET /scan.bin?dec=&rate=&since=`, which answers a 24 B little endian
preamble (`"SCN2"`, pixels per line, lines in this batch, id of the first, dpi,
scan rate, published rate, lines dropped, flags, decimation) followed by that
many lines of interleaved RGB. The **browser** accumulates the waterfall in a
canvas; the device only keeps the last few lines.

**One request carries a batch, not a line.** The sensor scans at ~1000 lines/s
and a browser cannot make 1000 requests per second, so the firmware keeps a ring
sized in bytes (32 KB) and hands over everything published since the client's
`since` id. The ring therefore holds a dozen lines at 1/4 resolution, three at
full resolution and two dozen at 1/8: **resolution is paid for in line rate**,
which is what the page's **SPEED** (25 / 100 / 250 lines/s, or max) and
**RESOLUTION** boxes trade against each other. The status strip states what is
actually achieved -- `250 lines/s of 1052 · 864 px · 640 kB/s` -- and counts
dropped lines when the client cannot keep up, so the two boxes can be tuned on
fact rather than on hope.

The ring is written lock free: the scan task never waits on the HTTP task, and a
line lapped while it is being sent costs one torn column out of hundreds. Lines
are published only while a page keeps polling, and only one line out of
`scan_rate / requested_rate`, so an unwatched device pays nothing and asking for
25 lines/s costs 25 memcpy per second, not 1000. The HTTP server still serves one
client at a time: the page stops polling when its tab is hidden, and each
connection is closed after 64 requests so another browser is never locked out for
more than a couple of seconds.

The CIS settings sit under the image, so a change is judged on the picture it
produces: **DPI** (200 / 400), **OVSP** (oversampling), **Hand** (which way the
device is held, for calibration) and **Start CIS Calibration** -- move the device
continuously over a white reference while it runs. The line rate is no longer a
field to read: the frame's readout shows it live.

#### IMU

`/imu.html` plots the accelerometer and the gyroscope as rolling traces (one
pixel per sample, X / Y / Z in the catalogue's cyan / magenta / amber) and lights
the three buttons as they are pressed. It polls `GET /getImu`, which answers one
sample as JSON: `acc` in g, `gyro` in dps, `temp` in °C, the sample `seq` and the
three button states. The polling rate is picked in the page; like the scan page
it stops polling when its tab is hidden.

The IMU settings sit under the charts: **Gyro** and **Accel** full scales -- the
very ranges the traces are drawn against, so the plot always says what the sensor
is set to -- and **Start IMU Calibration** (keep the device still for ~1.5 s).

#### NETWORK

Identity first -- name, serial number, MAC address and host link state
(bound / streaming), refreshed every two seconds -- then the addresses:

- **IP Address/Subnet Mask/Gateway**: static IPv4 configuration of the device
  (default `192.168.100.1` / `255.255.255.0` / `0.0.0.0`).
- **Dest IP Address / Stream Port**: where `LINE`/`HID` datagrams go while no
  host session is bound (default `192.168.100.10:55151`).
- **Link Port (SLP)**: control channel port the device listens on (default `55150`).
- **Stream w/o host**: keep streaming to *Dest IP* without a host session (ON by default).

> **Note**: After modifying network settings, click **Apply Network Settings**
> (the device reboots, and the page follows it to its new address).

#### GUI

What the OLED shows and when it sleeps: **Show IMU** (the IMU strip),
**Invert CIS Image**, screensaver **Timeout**, and the **Motion Acc / Motion
Gyro** thresholds that count as movement and keep the screen awake.

#### Administrator password

Every request that **changes** the device -- firmware upload, network settings,
factory reset, CIS/IMU/GUI settings -- requires HTTP Basic credentials, user
`admin`. Read-only `GET` endpoints stay open so passive monitoring is not
disturbed.

The password is **drawn at random on first boot**: there is no factory default,
so no device ships with a credential that is printed in this file. It is shown
on the boot screen, second line, until it is first used -- and nowhere else. It
is never served over the network, since a password the network can read would
protect nothing. If it is lost, a **Factory Reset** generates and displays a new
one.

#### Firmware Update

To update the firmware via the HTTP interface, on the **UPDATE** tab
(`/update.html`, which also shows the installed version, hardware revision and
SLP protocol):

1. Select the firmware file from your local machine.
2. Click **Upload Firmware** and enter the administrator credentials.

The device verifies the package (header bounds and CRC-32) *before* rebooting:
an invalid package is answered `400` with the reason and costs no restart. Once
applied, the new image runs **on trial** -- it must prove the configuration was
read and the HTTP server is listening, and hold for 30 s, or the bootloader
restores the previous version automatically. See
[docs/PLAN_FIRMWARE_UPDATE.md](docs/PLAN_FIRMWARE_UPDATE.md).

#### Network flash and logs (no ST-Link)

For development, the bootloader carries a network flasher (UDP 55152, see
[docs/NETBOOT.md](docs/NETBOOT.md)): `POST /netboot` reboots the device into
it, `scripts/netboot/netflash.py flash --cm7 … --cm4 …` erases, writes,
verifies and reboots, in about the time the ST-Link needs. Holding the two
outer buttons at power-on enters the same mode when the application is not
reachable. All three images (bootloader, CM7, CM4) log into a ring in retained
RAM, served by `GET /log?src=cm7|cm4&since=N` on the application and by the
flasher while it runs: `scripts/netboot/netlog.py` follows it across reboots.

#### Advanced Settings

- **Factory Reset** (UPDATE tab):  
  Restores the device to its original factory settings, including a freshly drawn
  administrator password. The device reboots at the default address and the page
  follows it there.

### Files

The **FILES** tab (`/files.html`) is a read-only browser of the device storage
(configuration, calibration data, firmware packages). Directories open in
place, with the path shown above the listing; clicking a file downloads it.
Nothing can be written or deleted through this page -- the only write channel
remains the firmware upload on the UPDATE tab.

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
