#include "nexus_boot.h"
#include "nexus_crash.h"
#include "nexus_storage.h"
#include "nexus_time.h"
#include "nexus_log.h"
#include <stdio.h>
#include <string.h>

nexus_err_t nexus_boot_persist_crash(void) {
    if (!nexus_crash_has_record()) return NEXUS_ERR_NOT_FOUND;
    if (!nexus_storage_is_ready()) return NEXUS_ERR_STORAGE;

    char json[512];
    if (nexus_crash_format_json(json, sizeof(json)) != NEXUS_OK) {
        return NEXUS_ERR_GENERIC;
    }

    char stamp[24];
    nexus_datetime_t dt;
    if (nexus_time_is_valid() && nexus_time_now(&dt) == NEXUS_OK) {
        nexus_time_format_stamp(&dt, stamp, sizeof(stamp));
    } else {
        snprintf(stamp, sizeof(stamp), "NO_TIME");
    }

    char path[NEXUS_STORAGE_PATH_MAX];
    snprintf(path, sizeof(path), "%s/CRASHES/CRASH_%s.json", NEXUS_STORAGE_ROOT, stamp);

    nexus_err_t err = nexus_storage_write_atomic(path, (const uint8_t*)json, strlen(json));
    if (err == NEXUS_OK) {
        NEXUS_LOG_WARN("CRASH", "Previous crash persisted to %s", path);
        // The record has been safely externalised; clear it so it is not
        // reported twice.
        nexus_crash_clear();
    } else {
        NEXUS_LOG_ERROR("CRASH", "Failed to persist crash report (%d)", err);
    }
    return err;
}
