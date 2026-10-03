#include "nexus_test_engine.h"
#include "nexus_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static bool s_initialized = false;
static bool s_running = false;
static nexus_test_report_t s_last_report;

// Find a key within the bounded region [begin, end). This prevents a lookup
// for one step from matching a key that belongs to a later step.
static const char* nexus_json_find(const char* begin, const char* end, const char* key) {
    size_t key_len = strlen(key);
    if (key_len == 0 || begin == NULL || end == NULL) return NULL;
    for (const char* p = begin; p + key_len <= end; p++) {
        if (strncmp(p, key, key_len) == 0) {
            return p;
        }
    }
    return NULL;
}

nexus_err_t nexus_test_engine_init(void) {
    memset(&s_last_report, 0, sizeof(s_last_report));
    s_initialized = true;
    s_running = false;
    NEXUS_LOG_INFO("TEST", "Test engine initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_test_engine_load_test(const char* json_str, nexus_test_case_t* out_test) {
    if (!s_initialized || !json_str || !out_test) return NEXUS_ERR_INVALID_PARAM;

    // Simple JSON parser for test cases
    // In production, use a proper JSON library
    memset(out_test, 0, sizeof(nexus_test_case_t));

    // Parse test name
    const char* name_start = strstr(json_str, "\"name\"");
    if (name_start) {
        name_start = strchr(name_start, ':');
        if (name_start) {
            name_start++;
            while (*name_start == ' ' || *name_start == '"') name_start++;
            const char* name_end = strchr(name_start, '"');
            if (name_end) {
                size_t len = name_end - name_start;
                if (len >= NEXUS_TEST_NAME_LEN) len = NEXUS_TEST_NAME_LEN - 1;
                strncpy(out_test->name, name_start, len);
                out_test->name[len] = '\0';
            }
        }
    }

    // Parse power settings
    const char* power_start = strstr(json_str, "\"power\"");
    if (power_start) {
        const char* voltage_start = strstr(power_start, "\"voltage\"");
        if (voltage_start) {
            voltage_start = strchr(voltage_start, ':');
            if (voltage_start) {
                out_test->power_voltage = atof(voltage_start + 1);
                out_test->power_on = true;
            }
        }
    }

    // Parse each top-level object in the "tests" array.
    // We bound every field lookup to the current object so that keys are not
    // accidentally borrowed from a later step.
    const char* arr = strstr(json_str, "\"tests\"");
    if (arr) {
        arr = strchr(arr, '[');
    }
    if (arr) {
        const char* p = arr + 1;
        uint8_t step_count = 0;

        while (*p && *p != ']' && step_count < NEXUS_TEST_MAX_STEPS) {
            // Skip whitespace and separators between objects
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++;
            if (*p != '{') { if (*p) p++; continue; }

            // Find the matching closing brace for this object
            const char* obj_begin = p;
            int depth = 0;
            const char* obj_end = p;
            for (const char* q = p; *q; q++) {
                if (*q == '{') depth++;
                else if (*q == '}') { depth--; if (depth == 0) { obj_end = q; break; } }
            }

            nexus_test_step_t* step = &out_test->steps[step_count];
            memset(step, 0, sizeof(nexus_test_step_t));
            step->timeout_ms = 5000;

            // ---- type ----
            const char* type_start = nexus_json_find(obj_begin, obj_end, "\"type\"");
            if (type_start) {
                type_start = strchr(type_start, ':');
                if (type_start) {
                    type_start++;
                    while (*type_start == ' ' || *type_start == '"') type_start++;
                    if (strncmp(type_start, "delay", 5) == 0) step->type = NEXUS_TEST_STEP_DELAY;
                    else if (strncmp(type_start, "i2c_scan", 8) == 0) step->type = NEXUS_TEST_STEP_I2C_SCAN;
                    else if (strncmp(type_start, "uart_expect", 11) == 0) step->type = NEXUS_TEST_STEP_UART_EXPECT;
                    else if (strncmp(type_start, "uart_send", 9) == 0) step->type = NEXUS_TEST_STEP_UART_SEND;
                    else if (strncmp(type_start, "gpio_expect", 11) == 0) step->type = NEXUS_TEST_STEP_GPIO_EXPECT;
                    else if (strncmp(type_start, "gpio_set", 8) == 0) step->type = NEXUS_TEST_STEP_GPIO_SET;
                    else if (strncmp(type_start, "adc_read", 8) == 0) step->type = NEXUS_TEST_STEP_ADC_READ;
                    else if (strncmp(type_start, "power_set", 9) == 0) step->type = NEXUS_TEST_STEP_POWER_SET;
                    else if (strncmp(type_start, "power_measure", 13) == 0) step->type = NEXUS_TEST_STEP_POWER_MEASURE;
                    else if (strncmp(type_start, "spi_transfer", 12) == 0) step->type = NEXUS_TEST_STEP_SPI_TRANSFER;
                    else if (strncmp(type_start, "can_send", 8) == 0) step->type = NEXUS_TEST_STEP_CAN_SEND;
                    else if (strncmp(type_start, "can_receive", 11) == 0) step->type = NEXUS_TEST_STEP_CAN_RECEIVE;
                    else if (strncmp(type_start, "conditional", 11) == 0) step->type = NEXUS_TEST_STEP_CONDITIONAL;
                    else if (strncmp(type_start, "sequence", 8) == 0) step->type = NEXUS_TEST_STEP_SEQUENCE;
                }
            }

            // ---- delay (ms) ----
            const char* delay_start = nexus_json_find(obj_begin, obj_end, "\"ms\"");
            if (delay_start) {
                delay_start = strchr(delay_start, ':');
                if (delay_start) step->delay_ms = (uint32_t)atoi(delay_start + 1);
            }

            // ---- timeout ----
            const char* timeout_start = nexus_json_find(obj_begin, obj_end, "\"timeout\"");
            if (timeout_start) {
                timeout_start = strchr(timeout_start, ':');
                if (timeout_start) step->timeout_ms = (uint32_t)atoi(timeout_start + 1);
            }

            // ---- pin ----
            const char* pin_start = nexus_json_find(obj_begin, obj_end, "\"pin\"");
            if (pin_start) {
                pin_start = strchr(pin_start, ':');
                if (pin_start) {
                    pin_start++;
                    while (*pin_start == ' ' || *pin_start == '"') pin_start++;
                    if (*pin_start == 'D' || *pin_start == 'd') pin_start++;
                    step->pin = (uint8_t)atoi(pin_start);
                }
            }

            // ---- expected text ----
            const char* text_start = nexus_json_find(obj_begin, obj_end, "\"text\"");
            if (text_start) {
                text_start = strchr(text_start, ':');
                if (text_start) {
                    text_start++;
                    while (*text_start == ' ' || *text_start == '"') text_start++;
                    const char* text_end = strchr(text_start, '"');
                    if (text_end && text_end <= obj_end) {
                        size_t len = (size_t)(text_end - text_start);
                        if (len >= sizeof(step->expected_text)) len = sizeof(step->expected_text) - 1;
                        strncpy(step->expected_text, text_start, len);
                        step->expected_text[len] = '\0';
                    }
                }
            }

            step_count++;
            p = obj_end;
            if (*p) p++;
        }
        out_test->num_steps = step_count;
    }

    NEXUS_LOG_DEBUG("TEST", "Loaded test '%s' with %u steps", out_test->name, out_test->num_steps);
    return NEXUS_OK;
}

nexus_err_t nexus_test_engine_validate(const nexus_test_case_t* test) {
    if (!s_initialized || !test) return NEXUS_ERR_INVALID_PARAM;
    if (test->num_steps == 0) return NEXUS_ERR_INVALID_CONFIG;
    if (test->num_steps > NEXUS_TEST_MAX_STEPS) return NEXUS_ERR_INVALID_CONFIG;

    for (uint8_t i = 0; i < test->num_steps; i++) {
        const nexus_test_step_t* step = &test->steps[i];
        if (step->timeout_ms == 0) return NEXUS_ERR_INVALID_CONFIG;
        if (step->type == NEXUS_TEST_STEP_DELAY && step->delay_ms == 0) return NEXUS_ERR_INVALID_CONFIG;
    }

    return NEXUS_OK;
}

nexus_err_t nexus_test_engine_execute(const nexus_test_case_t* test, nexus_test_report_t* out_report) {
    if (!s_initialized || !test || !out_report) return NEXUS_ERR_INVALID_PARAM;
    if (s_running) return NEXUS_ERR_RESOURCE_BUSY;

    s_running = true;
    memset(out_report, 0, sizeof(nexus_test_report_t));
    strncpy(out_report->test_name, test->name, NEXUS_TEST_NAME_LEN - 1);
    out_report->steps_total = test->num_steps;

    NEXUS_LOG_INFO("TEST", "Starting test: %s", test->name);

    uint32_t start_time = 0; // Would use millis() on hardware

    for (uint8_t i = 0; i < test->num_steps; i++) {
        const nexus_test_step_t* step = &test->steps[i];
        NEXUS_LOG_INFO("TEST", "  Step %u: type=%d", i, step->type);

        // Simulate step execution
        switch (step->type) {
            case NEXUS_TEST_STEP_DELAY:
                // Would use delay() on hardware
                break;
            case NEXUS_TEST_STEP_I2C_SCAN:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_UART_EXPECT:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_GPIO_EXPECT:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_ADC_READ:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_POWER_SET:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_POWER_MEASURE:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_SPI_TRANSFER:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_CAN_SEND:
                out_report->steps_passed++;
                break;
            case NEXUS_TEST_STEP_CAN_RECEIVE:
                out_report->steps_passed++;
                break;
            default:
                out_report->steps_skipped++;
                break;
        }
    }

    uint32_t end_time = 100; // Would use millis() on hardware
    out_report->duration_ms = end_time - start_time;

    if (out_report->steps_failed == 0) {
        out_report->result = NEXUS_TEST_PASS;
    } else {
        out_report->result = NEXUS_TEST_FAIL;
    }

    // Generate report
    snprintf(out_report->report, sizeof(out_report->report),
        "WattLab Nexus\nAutomated DUT Test\n\nDevice:\n%s\n\nResult:\n%s\n\nTests:\n[%s] Step 1\n[%s] Step 2\n\nDuration:\n%u ms\n",
        test->name,
        out_report->result == NEXUS_TEST_PASS ? "PASS" : "FAIL",
        out_report->result == NEXUS_TEST_PASS ? "PASS" : "FAIL",
        out_report->result == NEXUS_TEST_PASS ? "PASS" : "FAIL",
        out_report->duration_ms);

    s_last_report = *out_report;
    s_running = false;

    NEXUS_LOG_INFO("TEST", "Test completed: %s (%u passed, %u failed, %u skipped)",
        out_report->result == NEXUS_TEST_PASS ? "PASS" : "FAIL",
        out_report->steps_passed, out_report->steps_failed, out_report->steps_skipped);

    return NEXUS_OK;
}

nexus_err_t nexus_test_engine_cancel(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    s_running = false;
    NEXUS_LOG_INFO("TEST", "Test cancelled");
    return NEXUS_OK;
}

bool nexus_test_engine_is_running(void) {
    return s_running;
}

const nexus_test_report_t* nexus_test_engine_get_last_report(void) {
    return &s_last_report;
}
