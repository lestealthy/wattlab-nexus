#include "nexus_storage.h"
#include "nexus_log.h"
#include <string.h>
#include <stdio.h>

static bool s_initialized = false;
static nexus_storage_backend_t* s_backend = NULL;
static nexus_storage_state_t s_state = NEXUS_STORAGE_NOT_PRESENT;
static nexus_storage_thresholds_t s_thresholds = { 80, 90, 95 };
static char s_last_error[64] = "";

static const char* const kLayoutDirs[] = {
    NEXUS_STORAGE_ROOT,
    NEXUS_STORAGE_ROOT "/CONFIG",
    NEXUS_STORAGE_ROOT "/DEVICES",
    NEXUS_STORAGE_ROOT "/TESTS",
    NEXUS_STORAGE_ROOT "/REPORTS",
    NEXUS_STORAGE_ROOT "/CAPTURES",
    NEXUS_STORAGE_ROOT "/LOGS",
    NEXUS_STORAGE_ROOT "/CRASHES",
    NEXUS_STORAGE_ROOT "/EXPORTS",
    NEXUS_STORAGE_ROOT "/FIRMWARE",
};

// --------------------------------------------------------------------------
// Path safety
// --------------------------------------------------------------------------
bool nexus_storage_path_is_safe(const char* path) {
    if (!path || path[0] != '/') return false;
    size_t len = strlen(path);
    if (len == 0 || len >= NEXUS_STORAGE_PATH_MAX) return false;

    // No backslashes, no control characters.
    for (size_t i = 0; i < len; i++) {
        char c = path[i];
        if (c == '\\') return false;
        if ((unsigned char)c < 0x20) return false;
    }

    // Reject ".." path segments and empty "//" segments.
    const char* p = path;
    while (*p) {
        if (*p == '/') {
            if (p[1] == '/' ) return false;           // empty segment
            if (p[1] == '.' && p[2] == '.') {
                // ".." must be followed by '/' or end
                if (p[3] == '\0' || p[3] == '/') return false;
            }
        }
        p++;
    }
    if (len > 1 && path[len - 1] == '/') return false; // trailing slash
    return true;
}

nexus_err_t nexus_storage_sanitize_name(const char* name, char* out, size_t out_size) {
    if (!name || !out || out_size == 0) return NEXUS_ERR_INVALID_PARAM;

    size_t j = 0;
    for (size_t i = 0; name[i] && j + 1 < out_size; i++) {
        char c = name[i];
        // Replace path separators and Windows-reserved characters.
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|' ||
            (unsigned char)c < 0x20) {
            c = '_';
        }
        out[j++] = c;
    }
    out[j] = '\0';
    if (j == 0) return NEXUS_ERR_INVALID_PARAM;
    return NEXUS_OK;
}

// --------------------------------------------------------------------------
// Lifecycle
// --------------------------------------------------------------------------
nexus_err_t nexus_storage_init(void) {
    s_initialized = true;
    s_state = NEXUS_STORAGE_NOT_PRESENT;
    s_last_error[0] = '\0';
    NEXUS_LOG_INFO("STORAGE", "Storage service initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_storage_select(nexus_storage_backend_t* backend) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    if (!backend) return NEXUS_ERR_INVALID_PARAM;
    s_backend = backend;
    NEXUS_LOG_INFO("STORAGE", "Backend selected: %s", backend->name ? backend->name : "?");
    return NEXUS_OK;
}

nexus_storage_backend_t* nexus_storage_active(void) { return s_backend; }

const char* nexus_storage_backend_name(void) {
    return (s_backend && s_backend->name) ? s_backend->name : "none";
}

nexus_err_t nexus_storage_mount(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    if (!s_backend || !s_backend->mount) return NEXUS_ERR_NOT_SUPPORTED;

    s_state = NEXUS_STORAGE_MOUNTING;
    nexus_err_t err = s_backend->mount();
    if (err != NEXUS_OK) {
        s_state = NEXUS_STORAGE_NOT_PRESENT;
        snprintf(s_last_error, sizeof(s_last_error), "mount failed (%d)", err);
        NEXUS_LOG_WARN("STORAGE", "Mount failed: %d", err);
        return err;
    }
    s_state = NEXUS_STORAGE_READY;
    NEXUS_LOG_INFO("STORAGE", "Mounted");

    // Best-effort layout creation; absence is not fatal.
    nexus_err_t le = nexus_storage_ensure_layout();
    if (le != NEXUS_OK) {
        NEXUS_LOG_WARN("STORAGE", "Layout creation issue: %d", le);
    }
    nexus_storage_recover_temp_files();
    return NEXUS_OK;
}

nexus_err_t nexus_storage_unmount(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    if (!s_backend || !s_backend->unmount) return NEXUS_ERR_NOT_SUPPORTED;
    s_state = NEXUS_STORAGE_BUSY;
    nexus_err_t err = s_backend->unmount();
    s_state = (err == NEXUS_OK) ? NEXUS_STORAGE_REMOVED : NEXUS_STORAGE_ERROR;
    NEXUS_LOG_INFO("STORAGE", "Unmounted (%d)", err);
    return err;
}

nexus_err_t nexus_storage_eject(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    NEXUS_LOG_INFO("STORAGE", "Eject requested: flushing and unmounting");
    return nexus_storage_unmount();
}

nexus_storage_state_t nexus_storage_state(void) { return s_state; }
bool nexus_storage_is_ready(void) { return s_state == NEXUS_STORAGE_READY; }

nexus_err_t nexus_storage_get_info(nexus_storage_info_t* info) {
    if (!s_initialized || !info) return NEXUS_ERR_INVALID_PARAM;
    memset(info, 0, sizeof(*info));
    if (!s_backend || !s_backend->info || s_state != NEXUS_STORAGE_READY) {
        return NEXUS_ERR_NOT_FOUND;
    }
    return s_backend->info(info);
}

const char* nexus_storage_state_string(nexus_storage_state_t state) {
    switch (state) {
        case NEXUS_STORAGE_NOT_PRESENT: return "NOT PRESENT";
        case NEXUS_STORAGE_DETECTING:   return "DETECTING";
        case NEXUS_STORAGE_MOUNTING:    return "MOUNTING";
        case NEXUS_STORAGE_READY:       return "READY";
        case NEXUS_STORAGE_BUSY:        return "BUSY";
        case NEXUS_STORAGE_ERROR:       return "ERROR";
        case NEXUS_STORAGE_REMOVED:     return "REMOVED";
        case NEXUS_STORAGE_CORRUPTED:   return "CORRUPTED";
        default:                        return "UNKNOWN";
    }
}

// --------------------------------------------------------------------------
// Filesystem structure
// --------------------------------------------------------------------------
nexus_err_t nexus_storage_ensure_layout(void) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;
    if (!s_backend || !s_backend->mkdir) return NEXUS_ERR_NOT_SUPPORTED;

    uint32_t created = 0;
    for (size_t i = 0; i < sizeof(kLayoutDirs) / sizeof(kLayoutDirs[0]); i++) {
        const char* dir = kLayoutDirs[i];
        if (!s_backend->exists || !s_backend->exists(dir)) {
            nexus_err_t err = s_backend->mkdir(dir);
            if (err == NEXUS_OK) {
                created++;
            } else {
                // Creating a missing directory must never be fatal.
                NEXUS_LOG_WARN("STORAGE", "Could not create %s (%d)", dir, err);
            }
        }
    }
    if (created) NEXUS_LOG_INFO("STORAGE", "Created %u Nexus directories", created);
    return NEXUS_OK;
}

// Report and clean up any leftover .tmp files from an interrupted write.
nexus_err_t nexus_storage_recover_temp_files(void) {
    if (!s_initialized || !s_backend || !s_backend->list) return NEXUS_ERR_NOT_SUPPORTED;

    static const char* const dirs[] = {
        NEXUS_STORAGE_ROOT "/CONFIG",
        NEXUS_STORAGE_ROOT "/REPORTS",
        NEXUS_STORAGE_ROOT "/CAPTURES",
    };
    nexus_file_entry_t entries[32];
    uint32_t found_tmp = 0;

    for (size_t d = 0; d < sizeof(dirs) / sizeof(dirs[0]); d++) {
        uint32_t count = 0;
        if (s_backend->list(dirs[d], entries, 32, &count) != NEXUS_OK) continue;
        for (uint32_t i = 0; i < count; i++) {
            size_t n = strlen(entries[i].name);
            if (n > 4 && strcmp(entries[i].name + n - 4, ".tmp") == 0) {
                char full[NEXUS_STORAGE_PATH_MAX];
                snprintf(full, sizeof(full), "%s/%s", dirs[d], entries[i].name);
                if (s_backend->remove) s_backend->remove(full);
                found_tmp++;
            }
        }
    }
    if (found_tmp) {
        NEXUS_LOG_WARN("STORAGE", "Recovered %u incomplete .tmp file(s)", found_tmp);
    }
    return NEXUS_OK;
}

// --------------------------------------------------------------------------
// Quota
// --------------------------------------------------------------------------
void nexus_storage_set_thresholds(const nexus_storage_thresholds_t* t) {
    if (!t) return;
    s_thresholds = *t;
}

uint8_t nexus_storage_used_percent(void) {
    nexus_storage_info_t info;
    if (nexus_storage_get_info(&info) != NEXUS_OK) return 0;
    if (info.total_bytes == 0) return 0;
    return (uint8_t)((info.used_bytes * 100ULL) / info.total_bytes);
}

// --------------------------------------------------------------------------
// File operations
// --------------------------------------------------------------------------
static bool backend_ready(void) {
    return s_initialized && s_backend && s_state == NEXUS_STORAGE_READY;
}

bool nexus_storage_exists(const char* path) {
    if (!backend_ready() || !s_backend->exists) return false;
    if (!nexus_storage_path_is_safe(path)) return false;
    return s_backend->exists(path);
}

nexus_err_t nexus_storage_mkdir(const char* path) {
    if (!backend_ready() || !s_backend->mkdir) return NEXUS_ERR_NOT_SUPPORTED;
    if (!nexus_storage_path_is_safe(path)) return NEXUS_ERR_INVALID_PARAM;
    return s_backend->mkdir(path);
}

nexus_err_t nexus_storage_remove(const char* path) {
    if (!backend_ready() || !s_backend->remove) return NEXUS_ERR_NOT_SUPPORTED;
    if (!nexus_storage_path_is_safe(path)) return NEXUS_ERR_INVALID_PARAM;
    return s_backend->remove(path);
}

nexus_err_t nexus_storage_rename(const char* from, const char* to) {
    if (!backend_ready() || !s_backend->rename) return NEXUS_ERR_NOT_SUPPORTED;
    if (!nexus_storage_path_is_safe(from) || !nexus_storage_path_is_safe(to)) return NEXUS_ERR_INVALID_PARAM;
    return s_backend->rename(from, to);
}

nexus_err_t nexus_storage_read_file(const char* path, uint8_t* buffer, size_t buffer_size, size_t* bytes_read) {
    if (bytes_read) *bytes_read = 0;
    if (!backend_ready() || !s_backend->open || !buffer || !bytes_read) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_storage_path_is_safe(path)) return NEXUS_ERR_INVALID_PARAM;

    nexus_file_t* f = s_backend->open(path, NEXUS_FILE_READ);
    if (!f) return NEXUS_ERR_NOT_FOUND;
    int n = s_backend->read(f, buffer, buffer_size);
    s_backend->close(f);
    if (n < 0) return NEXUS_ERR_STORAGE;
    *bytes_read = (size_t)n;
    return NEXUS_OK;
}

nexus_err_t nexus_storage_write_file(const char* path, const uint8_t* data, size_t len) {
    if (!backend_ready() || !s_backend->open || !data) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_storage_path_is_safe(path)) return NEXUS_ERR_INVALID_PARAM;

    nexus_file_t* f = s_backend->open(path, NEXUS_FILE_WRITE);
    if (!f) return NEXUS_ERR_STORAGE;
    size_t w = s_backend->write(f, data, len);
    s_backend->flush(f);
    s_backend->close(f);
    return (w == len) ? NEXUS_OK : NEXUS_ERR_STORAGE;
}

nexus_err_t nexus_storage_append_file(const char* path, const uint8_t* data, size_t len) {
    if (!backend_ready() || !s_backend->open || !data) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_storage_path_is_safe(path)) return NEXUS_ERR_INVALID_PARAM;

    nexus_file_t* f = s_backend->open(path, NEXUS_FILE_APPEND);
    if (!f) return NEXUS_ERR_STORAGE;
    size_t w = s_backend->write(f, data, len);
    s_backend->flush(f);
    s_backend->close(f);
    return (w == len) ? NEXUS_OK : NEXUS_ERR_STORAGE;
}

nexus_err_t nexus_storage_list(const char* dir, nexus_file_entry_t* out, uint32_t max, uint32_t* count) {
    if (count) *count = 0;
    if (!backend_ready() || !s_backend->list || !out || !count) return NEXUS_ERR_INVALID_PARAM;
    if (!nexus_storage_path_is_safe(dir)) return NEXUS_ERR_INVALID_PARAM;
    return s_backend->list(dir, out, max, count);
}

nexus_file_t* nexus_storage_open(const char* path, uint32_t mode) {
    if (!backend_ready() || !s_backend->open) return NULL;
    if (!nexus_storage_path_is_safe(path)) return NULL;
    return s_backend->open(path, mode);
}

int nexus_storage_read(nexus_file_t* f, void* buf, size_t len) {
    if (!backend_ready() || !f || !s_backend->read) return -1;
    return s_backend->read(f, buf, len);
}

size_t nexus_storage_write(nexus_file_t* f, const void* buf, size_t len) {
    if (!backend_ready() || !f || !s_backend->write) return 0;
    return s_backend->write(f, buf, len);
}

bool nexus_storage_seek(nexus_file_t* f, uint32_t pos) {
    if (!backend_ready() || !f || !s_backend->seek) return false;
    return s_backend->seek(f, pos);
}

uint32_t nexus_storage_tell(nexus_file_t* f) {
    if (!backend_ready() || !f || !s_backend->tell) return 0;
    return s_backend->tell(f);
}

uint32_t nexus_storage_file_size(nexus_file_t* f) {
    if (!backend_ready() || !f || !s_backend->size) return 0;
    return s_backend->size(f);
}

void nexus_storage_flush(nexus_file_t* f) {
    if (!backend_ready() || !f || !s_backend->flush) return;
    s_backend->flush(f);
}

void nexus_storage_close(nexus_file_t* f) {
    if (!f || !s_backend || !s_backend->close) return;
    s_backend->close(f);
}

nexus_err_t nexus_storage_write_atomic(const char* path, const uint8_t* data, size_t len) {
    if (!nexus_storage_path_is_safe(path)) return NEXUS_ERR_INVALID_PARAM;
    char tmp[NEXUS_STORAGE_PATH_MAX];
    int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return NEXUS_ERR_INVALID_PARAM;

    nexus_err_t err = nexus_storage_write_file(tmp, data, len);
    if (err != NEXUS_OK) {
        nexus_storage_remove(tmp);
        return err;
    }
    err = nexus_storage_rename(tmp, path);
    if (err != NEXUS_OK) {
        nexus_storage_remove(tmp);
        return err;
    }
    return NEXUS_OK;
}

// --------------------------------------------------------------------------
// Diagnostics
// --------------------------------------------------------------------------
nexus_err_t nexus_storage_diag_read_test(nexus_storage_diagnostics_t* out) {
    if (!out) return NEXUS_ERR_INVALID_PARAM;
    memset(out, 0, sizeof(*out));
    out->state = s_state;
    out->detected = (s_state == NEXUS_STORAGE_READY);
    out->mounted = (s_state == NEXUS_STORAGE_READY);
    strncpy(out->last_error, s_last_error, sizeof(out->last_error) - 1);

    nexus_storage_info_t info;
    if (nexus_storage_get_info(&info) == NEXUS_OK) {
        out->fat_type = info.fat_type;
        out->total_bytes = info.total_bytes;
        out->free_bytes = info.free_bytes;
    }

    // Non-destructive: list the root.
    if (backend_ready() && s_backend->list) {
        nexus_file_entry_t e[8];
        uint32_t c = 0;
        out->read_ok = (s_backend->list(NEXUS_STORAGE_ROOT, e, 8, &c) == NEXUS_OK);
    }
    return NEXUS_OK;
}

nexus_err_t nexus_storage_diag_write_test(nexus_storage_diagnostics_t* out) {
    if (!out) return NEXUS_ERR_INVALID_PARAM;
    nexus_storage_diag_read_test(out);
    if (!backend_ready()) return NEXUS_OK; // report state only, not an error

    // Explicit write test - creates and removes a probe file.
    const char* probe = NEXUS_STORAGE_ROOT "/EXPORTS/.nexus_write_test";
    const char* payload = "nexus-write-test";
    nexus_err_t err = nexus_storage_write_file(probe, (const uint8_t*)payload, strlen(payload));
    out->write_ok = (err == NEXUS_OK);
    if (out->write_ok) nexus_storage_remove(probe);
    return NEXUS_OK;
}
