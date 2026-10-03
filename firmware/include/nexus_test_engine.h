#ifndef NEXUS_TEST_ENGINE_H
#define NEXUS_TEST_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

#define NEXUS_TEST_MAX_STEPS 32
#define NEXUS_TEST_NAME_LEN 64
#define NEXUS_TEST_MAX_ASSERTIONS 8

typedef enum {
    NEXUS_TEST_PASS = 0,
    NEXUS_TEST_FAIL,
    NEXUS_TEST_SKIPPED,
    NEXUS_TEST_ERROR,
    NEXUS_TEST_TIMEOUT,
} nexus_test_result_t;

typedef enum {
    NEXUS_TEST_STEP_DELAY = 0,
    NEXUS_TEST_STEP_I2C_SCAN,
    NEXUS_TEST_STEP_I2C_READ,
    NEXUS_TEST_STEP_I2C_WRITE,
    NEXUS_TEST_STEP_UART_EXPECT,
    NEXUS_TEST_STEP_UART_SEND,
    NEXUS_TEST_STEP_GPIO_EXPECT,
    NEXUS_TEST_STEP_GPIO_SET,
    NEXUS_TEST_STEP_ADC_READ,
    NEXUS_TEST_STEP_POWER_SET,
    NEXUS_TEST_STEP_POWER_MEASURE,
    NEXUS_TEST_STEP_SPI_TRANSFER,
    NEXUS_TEST_STEP_CAN_SEND,
    NEXUS_TEST_STEP_CAN_RECEIVE,
    NEXUS_TEST_STEP_CONDITIONAL,
    NEXUS_TEST_STEP_SEQUENCE,
} nexus_test_step_type_t;

typedef struct {
    nexus_test_step_type_t type;
    char name[NEXUS_TEST_NAME_LEN];
    uint32_t timeout_ms;
    uint32_t delay_ms;
    uint8_t pin;
    uint8_t channel;
    uint8_t i2c_address;
    uint8_t i2c_reg;
    uint8_t uart_baud;
    uint8_t gpio_state;
    uint8_t power_state;
    float power_voltage;
    float power_current_limit;
    uint8_t spi_mode;
    uint32_t spi_freq;
    uint32_t can_id;
    uint8_t can_dlc;
    uint8_t can_data[8];
    char expected_text[32];
    uint8_t expected_data[16];
    uint8_t expected_len;
    float expected_value;
    float tolerance;
    uint8_t condition_step;
    uint8_t condition_result;
} nexus_test_step_t;

typedef struct {
    char name[NEXUS_TEST_NAME_LEN];
    uint8_t num_steps;
    nexus_test_step_t steps[NEXUS_TEST_MAX_STEPS];
    bool power_on;
    float power_voltage;
    uint32_t power_current_limit;
} nexus_test_case_t;

typedef struct {
    char test_name[NEXUS_TEST_NAME_LEN];
    nexus_test_result_t result;
    uint32_t duration_ms;
    uint8_t steps_passed;
    uint8_t steps_failed;
    uint8_t steps_skipped;
    uint8_t steps_total;
    char report[512];
} nexus_test_report_t;

nexus_err_t nexus_test_engine_init(void);
nexus_err_t nexus_test_engine_load_test(const char* json_str, nexus_test_case_t* out_test);
nexus_err_t nexus_test_engine_validate(const nexus_test_case_t* test);
nexus_err_t nexus_test_engine_execute(const nexus_test_case_t* test, nexus_test_report_t* out_report);
nexus_err_t nexus_test_engine_cancel(void);
bool nexus_test_engine_is_running(void);
const nexus_test_report_t* nexus_test_engine_get_last_report(void);

#endif
