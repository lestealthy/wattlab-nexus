#ifndef NEXUS_STORAGE_H
#define NEXUS_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nexus_errors.h"

/*
 * Storage service.
 *
 * Layering:
 *
 *     Application / Logger / Capture / Reports
 *                     |
 *                     v
 *              nexus_storage (facade: path safety, state, routing)
 *                     |
 *          +----------+-----------+
 *          v                      v
 *   SDCardBackend (device)   HostFilesystemBackend (host tests)
 *
 * The rest of Nexus must never call the SD/FatFs library directly. It uses the
 * facade in this header. This keeps host tests and the device build on the
 * same code path.
 */

#define NEXUS_STORAGE_ROOT       "/NEXUS"
#define NEXUS_STORAGE_PATH_MAX   192
#define NEXUS_STORAGE_NAME_MAX   64

typedef enum {
    NEXUS_STORAGE_NOT_PRESENT = 0,
    NEXUS_STORAGE_DETECTING,
    NEXUS_STORAGE_MOUNTING,
    NEXUS_STORAGE_READY,
    NEXUS_STORAGE_BUSY,
    NEXUS_STORAGE_ERROR,
    NEXUS_STORAGE_REMOVED,
    NEXUS_STORAGE_CORRUPTED,
} nexus_storage_state_t;

typedef struct {
    bool mounted;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint64_t used_bytes;
    char label[16];
    uint8_t fat_type;   // 0 unknown, 12, 16, 32, 64
} nexus_storage_info_t;

typedef struct {
    char name[NEXUS_STORAGE_NAME_MAX];
    uint32_t size;
    bool is_directory;
    uint32_t modified;  // unix seconds, 0 if unknown
} nexus_file_entry_t;

// File open modes (combinable).
typedef enum {
    NEXUS_FILE_READ   = (1 << 0),
    NEXUS_FILE_WRITE  = (1 << 1),  // create + truncate
    NEXUS_FILE_APPEND = (1 << 2),  // create + seek to end
} nexus_file_mode_t;

// Opaque open file. Backends interpret `handle`.
typedef struct {
    void* handle;
    uint32_t mode;
} nexus_file_t;

// Backend vtable. All paths are logical (e.g. "/NEXUS/LOGS/x.log"); each
// backend maps them to its own root.
typedef struct {
    const char* name;
    nexus_err_t (*mount)(void);
    nexus_err_t (*unmount)(void);
    nexus_storage_state_t (*state)(void);
    nexus_err_t (*info)(nexus_storage_info_t* out);
    bool (*exists)(const char* path);
    nexus_err_t (*mkdir)(const char* path);
    nexus_err_t (*remove)(const char* path);
    nexus_err_t (*rename)(const char* from, const char* to);
    nexus_file_t* (*open)(const char* path, uint32_t mode);
    int (*read)(nexus_file_t* f, void* buf, size_t len);
    size_t (*write)(nexus_file_t* f, const void* buf, size_t len);
    bool (*seek)(nexus_file_t* f, uint32_t pos);
    uint32_t (*tell)(nexus_file_t* f);
    uint32_t (*size)(nexus_file_t* f);
    void (*flush)(nexus_file_t* f);
    void (*close)(nexus_file_t* f);
    nexus_err_t (*list)(const char* dir, nexus_file_entry_t* out, uint32_t max, uint32_t* count);
} nexus_storage_backend_t;

// ---- Backend selection -----------------------------------------------------
nexus_err_t nexus_storage_init(void);
nexus_err_t nexus_storage_select(nexus_storage_backend_t* backend);
nexus_storage_backend_t* nexus_storage_active(void);
const char* nexus_storage_backend_name(void);

// ---- State / lifecycle -----------------------------------------------------
nexus_err_t nexus_storage_mount(void);
nexus_err_t nexus_storage_unmount(void);
nexus_err_t nexus_storage_eject(void);   // flush + unmount (safe-to-remove)
nexus_storage_state_t nexus_storage_state(void);
bool nexus_storage_is_ready(void);
nexus_err_t nexus_storage_get_info(nexus_storage_info_t* info);
const char* nexus_storage_state_string(nexus_storage_state_t state);

// ---- Filesystem structure --------------------------------------------------
// Creates /NEXUS and the standard subdirectories. Missing dirs are not an error.
nexus_err_t nexus_storage_ensure_layout(void);
nexus_err_t nexus_storage_recover_temp_files(void);

// ---- Quota / thresholds ----------------------------------------------------
typedef struct {
    uint8_t warn_percent;
    uint8_t critical_percent;
    uint8_t full_percent;
} nexus_storage_thresholds_t;
void nexus_storage_set_thresholds(const nexus_storage_thresholds_t* t);
uint8_t nexus_storage_used_percent(void);

// ---- Path safety -----------------------------------------------------------
// Returns true if `path` is a safe logical Nexus path (no traversal, no
// backslashes, no control chars, bounded length).
bool nexus_storage_path_is_safe(const char* path);
// Copies `path` into `out`, sanitising unsafe characters in the final segment.
nexus_err_t nexus_storage_sanitize_name(const char* name, char* out, size_t out_size);

// ---- File operations -------------------------------------------------------
bool nexus_storage_exists(const char* path);
nexus_err_t nexus_storage_mkdir(const char* path);
nexus_err_t nexus_storage_remove(const char* path);
nexus_err_t nexus_storage_rename(const char* from, const char* to);
nexus_err_t nexus_storage_read_file(const char* path, uint8_t* buffer, size_t buffer_size, size_t* bytes_read);
nexus_err_t nexus_storage_write_file(const char* path, const uint8_t* data, size_t len);
nexus_err_t nexus_storage_append_file(const char* path, const uint8_t* data, size_t len);
nexus_err_t nexus_storage_list(const char* dir, nexus_file_entry_t* out, uint32_t max, uint32_t* count);

nexus_file_t* nexus_storage_open(const char* path, uint32_t mode);
int nexus_storage_read(nexus_file_t* f, void* buf, size_t len);
size_t nexus_storage_write(nexus_file_t* f, const void* buf, size_t len);
bool nexus_storage_seek(nexus_file_t* f, uint32_t pos);
uint32_t nexus_storage_tell(nexus_file_t* f);
uint32_t nexus_storage_file_size(nexus_file_t* f);
void nexus_storage_flush(nexus_file_t* f);
void nexus_storage_close(nexus_file_t* f);

// Atomic-ish write: write `<path>.tmp`, then rename to `<path>`.
nexus_err_t nexus_storage_write_atomic(const char* path, const uint8_t* data, size_t len);

// ---- Diagnostics -----------------------------------------------------------
typedef struct {
    nexus_storage_state_t state;
    bool detected;
    bool mounted;
    uint8_t fat_type;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint32_t read_test_ms;
    uint32_t write_test_ms;
    bool read_ok;
    bool write_ok;
    char last_error[64];
} nexus_storage_diagnostics_t;

// Non-destructive read test; write test is explicit and opt-in.
nexus_err_t nexus_storage_diag_read_test(nexus_storage_diagnostics_t* out);
nexus_err_t nexus_storage_diag_write_test(nexus_storage_diagnostics_t* out);

#endif
