# Spectral Sound Scanner CIS Firmware

![Spectral Sound Scanner](https://reso-nance.org/wp-content/uploads/2023/06/20230709_135345-1140x624.jpg)

The Spectral Sound Scanner (SSS) is an innovative tangible interface for creating music and visuals. Utilizing our CIS instrument, the SSS produces a UDP stream of thousands of values captured by contact image sensor, transmitted at a frequency up to 1000 Hz. This stream can be used to generate music or display image stream in Max/MSP, Pure Data, and our [Viewer](https://github.com/Ondulab/SSS_Viewer).

## Features

- **UDP Communication:** Real-time transmission at 200 DPI with a frequency of 1000 Hz for an immersive audiovisual capture.
- **Network Configuration:** The SSS is designed for easy integration into any network setup, with adjustable settings to accommodate various environments.
- **Power over Ethernet (PoE):** The device supports PoE for simplified wiring and handling, allowing both power and data to be carried over a single Ethernet cable.
- **User Control:** Interface buttons allow for fine-tuning of sampling frequency and other parameters, providing intuitive user interaction.

## UDP Communication and Network Configuration

The Spectral Sound Scanner employs UDP communication to transmit data captured by the CIS. Here are the technical details of the transmission:

# Features

## Power over Ethernet (PoE)

Our device now supports Power over Ethernet (PoE), which simplifies cabling and installation by allowing both electrical power and data transfer over a single Ethernet cable.

### Hardware Prerequisites

- PoE-compatible network switch conforming to IEEE 802.3af/at standards.
- PoE injectors (if your switch does not support PoE) capable of delivering the necessary power.

### Configuration

- Ensure the PoE switch is configured to supply adequate power to the ports in use.
- Connect the device to the PoE switch using a Category 5e or higher Ethernet cable.

## Inertial Measurement Unit (IMU)

The integration of an inertial measurement unit enhances accuracy in motion capture and spatial orientation.

### IMU Features

- Sampling frequency: 1000 Hz
- High precision for acceleration and rotation measurement
- Real-time communication via UDP

## User Interface

Interface buttons are provided for fine-tuning the sampling frequency and other parameters.

### Usage

- Press the buttons to increase or decrease the sampling frequency.
- Adjusted settings can be saved and will persist after a reboot.

To correctly receive and assemble the UDP data:

1. Listen on the configured UDP port for the data stream.
2. Receive packets and use the 32-bit header to sequence the segments.
3. Combine the 6 segments to reconstruct the full line of 1728 pixels.
4. Repeat the process for each line transmitted by the CIS.

For more information on the Spectral Sound Scanner and other innovative projects, visit our website at [Réso-nance Numérique](https://reso-nance.org/).

## Installation

To install the firmware on your Spectral Sound Scanner:

1. Clone the repository to your local system.
2. Follow the network configuration instructions to enable UDP communication with your device.
3. Load the firmware onto the CIS following the detailed installation guide.

## Contributions

Contributions to this project are welcome. Please submit your pull requests or issues via GitHub.

## License

This project is distributed under the terms of the GNU General Public License. The license is available in the repository.

## Contact Us

For an opportunity to test our products, please reach out to us.
