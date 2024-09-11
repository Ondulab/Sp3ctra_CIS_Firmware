# CISYNTH CONTACT IMAGE SYNTHESIZER MAX8 External

![Spectral Sound Scanner](https://reso-nance.org/wp-content/uploads/2023/06/20230709_135345-1140x624.jpg)

## Description

The CYSINTH is an innovative tangible interface for creating music and visuals. Utilizing our CIS instrument, the CISYNTH generates a UDP stream containing thousands of values captured by both the contact image sensor and the inertial measurement unit, transmitted at frequencies of up to 1kHz. This stream can be used to generate music or display image streams in Max/MSP, Pure Data, and our [Viewer](https://github.com/Ondulab/CISYNTH_Viewer).

## Project Status

This project is now in an advanced stage of maturity, nearing the product’s commercialization. Please note that while most features are complete, minor adjustments may still be made as we prepare for the final release.

## Features

### Power over Ethernet (PoE)

Our device supports Power over Ethernet (PoE), which simplifies cabling and installation by allowing both electrical power and data transfer over a single Ethernet cable.

### Inertial Measurement Unit (IMU)

The integration of an inertial measurement unit enables precise 3D gesture tracking, allowing for detailed interaction.

### HTTP Server

The CISYNTH device comes with a built-in HTTP server that allows for easy configuration via a web browser. Access the web interface by navigating to the device's IP address (default: [192.168.0.10](http://192.168.0.10/config.html)). Below are the key sections and functionalities of the interface:

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

- **IP Address/Subnet Mask/Gateway**:  
  Allows the configuration of the Ethernet network settings, including:
  - IP Address (default: `192.168.0.10`)
  - Subnet Mask (default: `255.255.255.0`)
  - Gateway (default: `0.0.0.0`)

- **Destination IP Address (UDP)**:  
  Specifies the IP address of the target machine that will receive the UDP data packets.

- **UDP Port**:  
  Defines the port number for UDP packet transmission (default: `55151`).

> **Note**: After modifying network settings, click **Apply Network Settings** to apply the changes.

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
[Firmware](https://github.com/Ondulab/CISYNTH_CIS_Firmware), 
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
