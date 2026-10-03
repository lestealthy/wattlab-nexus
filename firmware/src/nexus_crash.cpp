#include "nexus_crash.h"
#include "nexus_log.h"
#include <string.h>
#include <stdio.h>

static nexus_crash_record_t s_record;
static bool s_initialized = false;

static uint32_t crash_checksum(const nexus_crash_record_t* r) {
    const uint8_t* p = (const uint8_t*)r;
    uint32_t sum = 0;
    // Exclude the trailing checksum field itself.
    for (size_t i = 0; i < sizeof(*r) - sizeof(uint32_t); i++) {
        sum = (sum << 1) | (sum >> 31);
        sum += p[i];
    }
    return sum;
}

nexus_err_t nexus_crash_init(void) {
    s_initialized = true;
    // A real port reads the persisted record from backup SRAM here. On the host
    // we start empty. nexus_crash_restore() can be used by a platform layer.
    memset(&s_record, 0, sizeof(s_record));
    return NEXUS_OK;
}

void nexus_crash_record(const char* task_name, uint32_t reset_reason) {
    if (!s_initialized) return;

    memset(&s_record, 0, sizeof(s_record));
    s_record.magic = NEXUS_CRASH_MAGIC;
    s_record.version = NEXUS_CRASH_VERSION;
    s_record.valid = 1;
    s_record.reset_reason = reset_reason;
    if (task_name) {
        strncpy(s_record.task_name, task_name, NEXUS_CRASH_TASK_LEN - 1);
        s_record.task_name[NEXUS_CRASH_TASK_LEN - 1] = '\0';
    }
    s_record.checksum = crash_checksum(&s_record);
}

bool nexus_crash_has_record(void) {
    if (!s_initialized) return false;
    if (s_record.magic != NEXUS_CRASH_MAGIC) return false;
    if (s_record.version != NEXUS_CRASH_VERSION) return false;
    if (!s_record.valid) return false;
    return s_record.checksum == crash_checksum(&s_record);
}

const nexus_crash_record_t* nexus_crash_get_record(void) {
    return &s_record;
}

nexus_err_t nexus_crash_clear(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    memset(&s_record, 0, sizeof(s_record));
    return NEXUS_OK;
}

const char* nexus_crash_reset_reason_string(uint32_t reason) {
    switch (reason) {
        case NEXUS_RESET_POWER_ON:  return "POWER_ON";
        case NEXUS_RESET_PIN:       return "PIN_RESET";
        case NEXUS_RESET_SOFTWARE:  return "SOFTWARE";
        case NEXUS_RESET_IWDG:      return "WATCHDOG_IWDG";
        case NEXUS_RESET_WWDG:      return "WATCHDOG_WWDG";
        case NEXUS_RESET_LOW_POWER: return "LOW_POWER";
        case NEXUS_RESET_BROWNOUT:  return "BROWNOUT";
        default:                    return "UNKNOWN";
    }
}

__attribute__((weak)) uint32_t nexus_crash_read_reset_reason(void) {
    return NEXUS_RESET_UNKNOWN;
}

nexus_err_t nexus_crash_format(char* buffer, size_t buffer_size) {
    if (!s_initialized || !buffer || buffer_size == 0) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_crash_has_record()) {
        snprintf(buffer, buffer_size, "No crash record.");
        return NEXUS_OK;
    }
    snprintf(buffer, buffer_size,
             "Last System Crash\n"
             "Task:   %s\n"
             "PC:     0x%08X\n"
             "LR:     0x%08X\n"
             "SP:     0x%08X\n"
             "CFSR:   0x%08X\n"
             "HFSR:   0x%08X\n"
             "Reset:  %s\n",
             s_record.task_name,
             (unsigned)s_record.pc, (unsigned)s_record.lr,
             (unsigned)s_record.sp, (unsigned)s_record.cfsr,
             (unsigned)s_record.hfsr,
             nexus_crash_reset_reason_string(s_record.reset_reason));
    return NEXUS_OK;
}

nexus_err_t nexus_crash_format_json(char* buffer, size_t buffer_size) {
    if (!s_initialized || !buffer || buffer_size == 0) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_crash_has_record()) {
        snprintf(buffer, buffer_size, "{}");
        return NEXUS_OK;
    }
    snprintf(buffer, buffer_size,
        "{\n"
        "  \"format\": \"nexus-crash\",\n"
        "  \"version\": %d,\n"
        "  \"task\": \"%s\",\n"
        "  \"reset_reason\": \"%s\",\n"
        "  \"pc\": \"0x%08X\",\n"
        "  \"lr\": \"0x%08X\",\n"
        "  \"sp\": \"0x%08X\",\n"
        "  \"cfsr\": \"0x%08X\",\n"
        "  \"hfsr\": \"0x%08X\",\n"
        "  \"mmfar\": \"0x%08X\",\n"
        "  \"bfar\": \"0x%08X\"\n"
        "}\n",
        NEXUS_CRASH_VERSION, s_record.task_name,
        nexus_crash_reset_reason_string(s_record.reset_reason),
        (unsigned)s_record.pc, (unsigned)s_record.lr, (unsigned)s_record.sp,
        (unsigned)s_record.cfsr, (unsigned)s_record.hfsr,
        (unsigned)s_record.mmfar, (unsigned)s_record.bfar);
    return NEXUS_OK;
}
