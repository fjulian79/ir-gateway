# IR Gateway

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

IR Gateway is an ESP32-based infrared remote control gateway designed to automate IR remote control commands in smart homes. It provides both a web interface and command-line interface for transmitting and receiving IR signals, making it perfect for integrating legacy IR-controlled devices into modern home automation systems.

## Features

- **IR Transmission**: Send IR codes using various protocols (NEC, Sony, RC5, etc.)
- **IR Reception**: Receive and decode IR signals from remote controls
- **Web Interface**: Simple HTTP API for remote control via web requests
- **Command Line Interface**: Interactive CLI for development and debugging
- **Sequence Support**: Execute multiple IR commands in sequence with configurable delays
- **WiFi Connectivity**: Connect to your home network with DHCP or static IP configuration
- **Logging**: Track transmission and reception history with configurable log sizes
- **mDNS**: Easy discovery via hostname resolution (e.g., `ir-gateway.local`)
- **NTP Time Sync**: Automatic time synchronization for accurate logging

## Hardware Requirements

### Supported Boards
- ESP32 DevKit
- Other ESP32 variants with minor pin adjustments

### Components
- ESP32 development board
- IR LED (for transmission)
- IR receiver module (e.g., TSOP4838, VS1838B)
- Resistors (220Ω for IR LED, 10kΩ pull-up for receiver)
- Breadboard or PCB for connections

### Pin Configuration

**Default ESP32:**
- IR TX Pin: GPIO 22
- IR RX Pin: GPIO 23  
- WiFi LED: GPIO 2

## Installation

### Prerequisites
- [PlatformIO](https://platformio.org/) installed
- ESP32 board definitions
- Git for cloning the repository

### Build and Upload

1. Clone the repository:
```bash
git clone https://github.com/fjulian79/ir-gateway.git
cd ir-gateway
```

2. Build and upload:
```bash
pio run -t upload
```

3. Monitor serial output:
```bash
pio device monitor
```

## Configuration

### Initial Setup

1. Connect to the device via serial terminal (115200 baud)
2. Configure WiFi and network parameters:

```
param set ssid
param set wifi-passwd
param set hostname
param save
```

### Parameter Commands

| Command | Description |
|---------|-------------|
| `param clear` | Reset all parameters to defaults |
| `param write` | Enter all parameters interactively |
| `param set [name]` | Set individual parameter |
| `param save` | Save parameters to flash |

### Available Parameters

- **ssid**: WiFi network name
- **wifi-passwd**: WiFi password
- **hostname**: Device hostname (default: ir-gateway)
- **dhcp**: Enable DHCP (true/false)
- **ipaddr**: Static IP address
- **netmask**: Network mask
- **gateway**: Gateway IP
- **ntp-server**: NTP server (default: pool.ntp.org)
- **timezone**: POSIX timezone string

## Usage

### Web Interface

Once connected to WiFi, access the device via:
- `http://[device-ip]`
- `http://ir-gateway.local` (if mDNS is working)

The web interface shows:
- Device status and uptime
- Transmission/reception statistics
- Links to logs and API endpoints

### Web API

#### Single IR Transmission
```
GET /tx?type=nec&code=0x1234&repeat=1
```

Parameters:
- `type`: IR protocol (nec, sony, rc5, etc.) - optional, defaults to NEC
- `code`: IR code in hex (0x1234) or decimal (4660)
- `repeat`: Number of repetitions (0-15) - optional, defaults to 0

#### Sequence Transmission
```
GET /txseq?sequence=nec:0x1234:1:500,sony:0x5678:2:1000
```

Format: `type:code:repeat:pause,type:code:repeat:pause,...`
- `pause`: Delay in milliseconds (optional, default 100ms)

#### Logs
- `GET /txlog`: View transmission log
- `GET /rxlog`: View reception log

### Command Line Interface

Connect via serial terminal for interactive control:

```
ver                             # Show version information
info                            # Display system information
tx nec 0x1234 1                 # Transmit IR code
txlog                           # Show transmission log
rxlog                           # Show reception log
networking 1                    # Enable/disable networking
reset                           # Restart device
help                            # Show all commands
```

## Examples

### Home Assistant Integration

```yaml
# configuration.yaml
rest_command:
  tv_power:
    url: "http://ir-gateway.local/tx?type=nec&code=0xC1AA09F6"
  
  tv_volume_up:
    url: "http://ir-gateway.local/tx?type=nec&code=0xC1AA49B6"
```

### Node-RED Integration

```javascript
// HTTP Request node
msg.url = "http://ir-gateway.local/tx";
msg.payload = {
    type: "nec",
    code: "0x1234",
    repeat: "1"
};
return msg;
```

### Python Script

```python
import requests

def send_ir_command(device_ip, protocol, code, repeat=0):
    url = f"http://{device_ip}/tx"
    params = {
        'type': protocol,
        'code': code,
        'repeat': repeat
    }
    response = requests.get(url, params=params)
    return response.text

# Usage
send_ir_command("192.168.1.100", "nec", "0x1234", 1)
```

## Supported IR Protocols

The device supports all protocols included in the IRremoteESP8266 library:
- NEC
- Sony
- RC5/RC6
- Samsung
- LG
- Panasonic
- And many more...

Use the `info` command to see all available protocols, or check the [IRremoteESP8266 documentation](https://github.com/crankyoldgit/IRremoteESP8266).

## Development

### Project Structure

```
ir-gateway/
├── src/
│   └── main.cpp              # Main application
├── lib/
│   ├── common/               # Common utilities
│   ├── ircontrol/            # IR transmission/reception
│   ├── parameter/            # Configuration management
│   ├── stringRingBuffer/     # Circular string buffer
│   └── webservercontrol/     # Web server handling
└── README.md
```

## Troubleshooting

### WiFi Connection Issues
- Verify SSID and password using `param set ssid` and `param set wifi-passwd`
- Check signal strength and network availability
- Try disabling/enabling networking: `networking 0` then `networking 1`

### IR Not Working
- Verify pin connections match your board configuration
- Check IR LED polarity and current limiting resistor
- Test with known working remote codes
- Use `rxlog` to verify reception is working

### Web Interface Not Accessible
- Confirm device is connected to WiFi (check serial output)
- Try IP address instead of hostname
- Verify firewall settings
- Check if mDNS is working on your network

### Common Error Messages
- "Invalid code value": Check hex format (must start with 0x)
- "Unknown type": Verify protocol name spelling
- "Timeout": WiFi connection failed, check credentials

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

See the [GPL v3 license](https://www.gnu.org/licenses/gpl-3.0) for details.

## Support

- **Issues**: [GitHub Issues](https://github.com/fjulian79/ir-gateway/issues)
- **Documentation**: This README and inline code comments
- **Community**: GitHub Discussions

## Acknowledgments

- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) library for IR protocol support
- ESP32 community for hardware support and examples
- Contributors and testers who helped improve the project
