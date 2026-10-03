#include "nexus_device_db.h"
#include "nexus_log.h"
#include <string.h>
#include <stdlib.h>

static nexus_device_info_t s_devices[NEXUS_DEVICE_DB_MAX_DEVICES];
static uint32_t s_device_count = 0;
static bool s_initialized = false;

// Built-in device database
static const nexus_device_info_t s_builtin_devices[] = {
    {
        .id = "bme280",
        .name = "Bosch BME280",
        .bus = NEXUS_BUS_I2C,
        .num_addresses = 2,
        .addresses = {0x76, 0x77},
        .identity_register = 0xD0,
        .identity_value = 0x60,
        .num_registers = 4,
        .registers = {
            {"TEMP_MSB", 0xFA, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
            {"TEMP_LSB", 0xFB, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
            {"PRES_MSB", 0xF7, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
            {"HUM_MSB", 0xFD, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
        },
        .capabilities = "temperature,pressure,humidity"
    },
    {
        .id = "mpu6050",
        .name = "InvenSense MPU6050",
        .bus = NEXUS_BUS_I2C,
        .num_addresses = 2,
        .addresses = {0x68, 0x69},
        .identity_register = 0x75,
        .identity_value = 0x68,
        .num_registers = 3,
        .registers = {
            {"ACCEL_XOUT_H", 0x3B, NEXUS_REG_INT16, 1, 1.0f, 0.0f, ""},
            {"ACCEL_YOUT_H", 0x3D, NEXUS_REG_INT16, 1, 1.0f, 0.0f, ""},
            {"ACCEL_ZOUT_H", 0x3F, NEXUS_REG_INT16, 1, 1.0f, 0.0f, ""},
        },
        .capabilities = "accelerometer,gyroscope,temperature"
    },
    {
        .id = "ina219",
        .name = "TI INA219",
        .bus = NEXUS_BUS_I2C,
        .num_addresses = 4,
        .addresses = {0x40, 0x41, 0x44, 0x45},
        .identity_register = 0x00,
        .identity_value = 0x21,
        .num_registers = 4,
        .registers = {
            {"SHUNT_VOLTAGE", 0x01, NEXUS_REG_INT16, 1, 1.0f, 0.0f, "mV"},
            {"BUS_VOLTAGE", 0x02, NEXUS_REG_UINT16, 1, 1.0f, 0.0f, "mV"},
            {"POWER", 0x03, NEXUS_REG_UINT16, 1, 1.0f, 0.0f, "mW"},
            {"CURRENT", 0x04, NEXUS_REG_INT16, 1, 1.0f, 0.0f, "mA"},
        },
        .capabilities = "voltage,current,power"
    },
    {
        .id = "ssd1306",
        .name = "SSD1306 OLED",
        .bus = NEXUS_BUS_I2C,
        .num_addresses = 2,
        .addresses = {0x3C, 0x3D},
        .identity_register = 0x00,
        .identity_value = 0x00,
        .num_registers = 1,
        .registers = {
            {"CONTRAST", 0x81, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
        },
        .capabilities = "display"
    },
    {
        .id = "ds3231",
        .name = "Maxim DS3231 RTC",
        .bus = NEXUS_BUS_I2C,
        .num_addresses = 1,
        .addresses = {0x68},
        .identity_register = 0x00,
        .identity_value = 0x00,
        .num_registers = 3,
        .registers = {
            {"SECONDS", 0x00, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
            {"MINUTES", 0x01, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
            {"HOURS", 0x02, NEXUS_REG_UINT8, 1, 1.0f, 0.0f, ""},
        },
        .capabilities = "rtc,time"
    },
    {
        .id = "ads1115",
        .name = "TI ADS1115 ADC",
        .bus = NEXUS_BUS_I2C,
        .num_addresses = 4,
        .addresses = {0x48, 0x49, 0x4A, 0x4B},
        .identity_register = 0x00,
        .identity_value = 0x00,
        .num_registers = 2,
        .registers = {
            {"CONVERSION", 0x00, NEXUS_REG_INT16, 1, 1.0f, 0.0f, "mV"},
            {"CONFIG", 0x01, NEXUS_REG_UINT16, 1, 1.0f, 0.0f, ""},
        },
        .capabilities = "adc,16bit"
    },
};

nexus_err_t nexus_device_db_init(void) {
    memset(s_devices, 0, sizeof(s_devices));
    s_device_count = 0;
    s_initialized = true;

    // Load built-in devices
    uint32_t builtin_count = sizeof(s_builtin_devices) / sizeof(s_builtin_devices[0]);
    for (uint32_t i = 0; i < builtin_count && i < NEXUS_DEVICE_DB_MAX_DEVICES; i++) {
        s_devices[s_device_count++] = s_builtin_devices[i];
    }

    NEXUS_LOG_INFO("DEVDB", "Device database initialized with %u devices", s_device_count);
    return NEXUS_OK;
}

nexus_err_t nexus_device_db_load(void) {
    NEXUS_LOG_INFO("DEVDB", "Loading device database from storage");
    return NEXUS_OK;
}

nexus_err_t nexus_device_db_save(void) {
    NEXUS_LOG_INFO("DEVDB", "Saving device database to storage");
    return NEXUS_OK;
}

const nexus_device_info_t* nexus_device_db_find_by_id(const char* id) {
    if (!s_initialized || !id) return NULL;

    for (uint32_t i = 0; i < s_device_count; i++) {
        if (strcmp(s_devices[i].id, id) == 0) {
            return &s_devices[i];
        }
    }
    return NULL;
}

const nexus_device_info_t* nexus_device_db_identify(nexus_bus_t bus, uint8_t address, uint8_t reg, uint8_t value) {
    if (!s_initialized) return NULL;

    for (uint32_t i = 0; i < s_device_count; i++) {
        const nexus_device_info_t* dev = &s_devices[i];
        if (dev->bus != bus) continue;
        if (dev->identity_register != reg) continue;
        if (dev->identity_value != value) continue;

        for (uint8_t a = 0; a < dev->num_addresses; a++) {
            if (dev->addresses[a] == address) {
                NEXUS_LOG_INFO("DEVDB", "Identified device: %s at 0x%02X", dev->name, address);
                return dev;
            }
        }
    }
    return NULL;
}

const nexus_device_info_t* nexus_device_db_get_all(uint32_t* count) {
    if (!s_initialized || !count) return NULL;
    *count = s_device_count;
    return s_devices;
}

nexus_err_t nexus_device_db_add(const nexus_device_info_t* device) {
    if (!s_initialized || !device) return NEXUS_ERR_INVALID_PARAM;
    if (s_device_count >= NEXUS_DEVICE_DB_MAX_DEVICES) return NEXUS_ERR_MEMORY;

    s_devices[s_device_count++] = *device;
    NEXUS_LOG_INFO("DEVDB", "Added device: %s", device->name);
    return NEXUS_OK;
}

nexus_err_t nexus_device_db_remove(const char* id) {
    if (!s_initialized || !id) return NEXUS_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < s_device_count; i++) {
        if (strcmp(s_devices[i].id, id) == 0) {
            // Shift remaining devices
            for (uint32_t j = i; j < s_device_count - 1; j++) {
                s_devices[j] = s_devices[j + 1];
            }
            s_device_count--;
            NEXUS_LOG_INFO("DEVDB", "Removed device: %s", id);
            return NEXUS_OK;
        }
    }
    return NEXUS_ERR_NOT_FOUND;
}
