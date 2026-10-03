# WattLab Nexus - Device Database

## Overview

The device database is an extensible, JSON-based system for identifying and describing devices connected to WattLab Nexus.

## Device Entry Format

```json
{
  "id": "bme280",
  "name": "Bosch BME280",
  "bus": "i2c",
  "addresses": ["0x76", "0x77"],
  "identity": {
    "register": "0xD0",
    "expected": ["0x60"]
  },
  "capabilities": ["temperature", "pressure", "humidity"],
  "registers": [
    {
      "name": "TEMP_MSB",
      "address": "0xFA",
      "type": "uint8",
      "scale": 1.0,
      "offset": 0.0,
      "unit": ""
    }
  ]
}
```

## Built-in Devices

| ID | Name | Bus | Addresses |
|----|------|-----|-----------|
| bme280 | Bosch BME280 | I2C | 0x76, 0x77 |
| mpu6050 | InvenSense MPU6050 | I2C | 0x68, 0x69 |
| ina219 | TI INA219 | I2C | 0x40, 0x41, 0x44, 0x45 |
| ssd1306 | SSD1306 OLED | I2C | 0x3C, 0x3D |
| ds3231 | Maxim DS3231 RTC | I2C | 0x68 |
| ads1115 | TI ADS1115 ADC | I2C | 0x48, 0x49, 0x4A, 0x4B |

## Adding Custom Devices

### Method 1: JSON File

Create a JSON file in the `data/devices/` directory on the SD card or QSPI flash.

### Method 2: Runtime API

```cpp
nexus_device_info_t my_device = {
    .id = "my_sensor",
    .name = "My Custom Sensor",
    .bus = NEXUS_BUS_I2C,
    .num_addresses = 1,
    .addresses = {0x50},
    .identity_register = 0x00,
    .identity_value = 0xAB,
    .num_registers = 1,
    .registers = {
        {"DATA", 0x01, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
    },
    .capabilities = "custom"
};
nexus_device_db_add(&my_device);
```

### Method 3: PC Companion

Use the PC companion application to upload device definitions via USB, Ethernet, or serial.

## Device Identification Process

1. **Bus Scan**: Scan all supported buses for active devices
2. **Identity Read**: Read identity register from each device
3. **Database Lookup**: Match register value against device database
4. **Capability Query**: Read device capabilities if available
5. **Profile Match**: Match against known device profiles

## Register Types

| Type | Description |
|------|-------------|
| uint8 | Unsigned 8-bit |
| int8 | Signed 8-bit |
| uint16 | Unsigned 16-bit |
| int16 | Signed 16-bit |
| uint32 | Unsigned 32-bit |
| int32 | Signed 32-bit |
| float | 32-bit IEEE 754 |
| bytes | Raw byte array |

## Scaling

Register values are scaled using:
```
physical_value = (raw_value * scale) + offset
```

Example: Temperature with scale=0.01, offset=-40.0
```
raw = 2500 -> physical = (2500 * 0.01) + (-40.0) = -15.0°C
```
