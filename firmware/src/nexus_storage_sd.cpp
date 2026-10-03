/*
 * SD card backend for the STM32F746G-DISCO, built on STM32duino STM32SD.
 *
 * Only compiled for the STM32 Arduino target. The host build uses the host
 * filesystem backend instead, so the storage service stays identical.
 */
#if defined(ARDUINO_ARCH_STM32)

#include "nexus_storage_sd.h"
#include "nexus_log.h"
#include <STM32SD.h>
#include <ff.h>
#include <string.h>
#include <stdio.h>

static bool s_sd_mounted = false;

// The STM32SD `SD` object exposes static File operations. We use those.
static bool sd_exists(const char* path) {
    return SD.exists(path);
}

static nexus_err_t sd_mkdir(const char* path) {
    if (SD.exists(path)) return NEXUS_OK;
    return SD.mkdir(path) ? NEXUS_OK : NEXUS_ERR_STORAGE;
}

static nexus_err_t sd_remove(const char* path) {
    return SD.remove(path) ? NEXUS_OK : NEXUS_ERR_STORAGE;
}

// Prefer FatFs f_rename for a true atomic rename (no copy, no data loss
// window). This is what makes the .tmp -> final atomic-write strategy real.
static nexus_err_t sd_rename(const char* from, const char* to) {
    FRESULT res = f_rename(from, to);
    if (res == FR_OK) return NEXUS_OK;
    return NEXUS_ERR_STORAGE;
}

static nexus_file_t* sd_open(const char* path, uint32_t mode) {
    uint8_t fsmode = FILE_READ;
    if (mode & NEXUS_FILE_WRITE) fsmode = FILE_WRITE;
    else if (mode & NEXUS_FILE_APPEND) fsmode = FILE_WRITE; // SD.open append not exposed

    File f = SD.open(path, fsmode);
    if (!f) return NULL;

    if (mode & NEXUS_FILE_APPEND) {
        f.seek(f.size());
    }

    // Wrap the File in a heap structure.
    File* held = new File(f);
    if (!held) { f.close(); return NULL; }
    nexus_file_t* out = (nexus_file_t*)calloc(1, sizeof(nexus_file_t));
    if (!out) { delete held; return NULL; }
    out->handle = held;
    out->mode = mode;
    return out;
}

static File* file_of(nexus_file_t* f) {
    return (f && f->handle) ? (File*)f->handle : NULL;
}

static int sd_read(nexus_file_t* f, void* buf, size_t len) {
    File* fp = file_of(f);
    if (!fp) return -1;
    return fp->read(buf, len);
}

static size_t sd_write(nexus_file_t* f, const void* buf, size_t len) {
    File* fp = file_of(f);
    if (!fp) return 0;
    return fp->write((const uint8_t*)buf, len);
}

static bool sd_seek(nexus_file_t* f, uint32_t pos) {
    File* fp = file_of(f);
    if (!fp) return false;
    return fp->seek(pos);
}

static uint32_t sd_tell(nexus_file_t* f) {
    File* fp = file_of(f);
    if (!fp) return 0;
    return fp->position();
}

static uint32_t sd_size(nexus_file_t* f) {
    File* fp = file_of(f);
    if (!fp) return 0;
    return fp->size();
}

static void sd_flush(nexus_file_t* f) {
    File* fp = file_of(f);
    if (fp) fp->flush();
}

static void sd_close(nexus_file_t* f) {
    File* fp = file_of(f);
    if (fp) { fp->close(); delete fp; }
    free(f);
}

static nexus_err_t sd_mount(void) {
    // SD.begin(detect, level) expects the card-detect PinName and its active
    // level. The variant defines SD_DETECT_PIN as PC13, active LOW.
    if (!SD.begin(SD_DETECT_PIN, LOW)) {
        s_sd_mounted = false;
        return NEXUS_ERR_NOT_FOUND;
    }
    s_sd_mounted = true;
    return NEXUS_OK;
}

static nexus_err_t sd_unmount(void) {
    SD.end();
    s_sd_mounted = false;
    return NEXUS_OK;
}

static nexus_storage_state_t sd_state(void) {
    if (!s_sd_mounted) return NEXUS_STORAGE_NOT_PRESENT;
    // A card that has been removed returns a stale/failed access; probe cheaply.
    if (!SD.exists("/")) return NEXUS_STORAGE_REMOVED;
    return NEXUS_STORAGE_READY;
}

static nexus_err_t sd_info(nexus_storage_info_t* out) {
    if (!out) return NEXUS_ERR_INVALID_PARAM;
    memset(out, 0, sizeof(*out));

    out->mounted = s_sd_mounted;
    out->fat_type = SD.fatType();

    // Card capacity via the BSP, which wraps HAL_SD_GetCardInfo.
    HAL_SD_CardInfoTypeDef card_info;
    if (BSP_SD_GetCardInfo(&card_info)) {
        out->total_bytes = (uint64_t)card_info.LogBlockNbr * (uint64_t)card_info.LogBlockSize;
    } else {
        out->total_bytes = 0;
    }

    // Authoritative free-space query via FatFs.
    DWORD free_clusters = 0;
    FATFS* fs = NULL;
    if (f_getfree("", &free_clusters, &fs) == FR_OK && fs != NULL) {
        uint64_t free_bytes = (uint64_t)free_clusters * fs->csize * 512ULL;
        if (free_bytes > out->total_bytes) free_bytes = out->total_bytes;
        out->free_bytes = free_bytes;
    } else {
        out->free_bytes = 0;
    }
    out->used_bytes = out->total_bytes - out->free_bytes;
    strncpy(out->label, "SD", sizeof(out->label) - 1);
    return NEXUS_OK;
}

static nexus_err_t sd_list(const char* dir, nexus_file_entry_t* out, uint32_t max, uint32_t* count) {
    if (!out || !count) return NEXUS_ERR_INVALID_PARAM;
    *count = 0;

    File d = SD.open(dir, FILE_READ);
    if (!d || !d.isDirectory()) return NEXUS_ERR_NOT_FOUND;

    File entry = d.openNextFile();
    while (entry && *count < max) {
        nexus_file_entry_t* e = &out[*count];
        memset(e, 0, sizeof(*e));
        strncpy(e->name, entry.name(), sizeof(e->name) - 1);
        e->is_directory = entry.isDirectory();
        e->size = entry.size();
        (*count)++;
        entry.close();
        entry = d.openNextFile();
    }
    d.close();
    return NEXUS_OK;
}

static nexus_storage_backend_t s_sd_backend = {
    .name = "sd",
    .mount = sd_mount,
    .unmount = sd_unmount,
    .state = sd_state,
    .info = sd_info,
    .exists = sd_exists,
    .mkdir = sd_mkdir,
    .remove = sd_remove,
    .rename = sd_rename,
    .open = sd_open,
    .read = sd_read,
    .write = sd_write,
    .seek = sd_seek,
    .tell = sd_tell,
    .size = sd_size,
    .flush = sd_flush,
    .close = sd_close,
    .list = sd_list,
};

nexus_storage_backend_t* nexus_storage_sd_backend(void) {
    return &s_sd_backend;
}

#endif // ARDUINO_ARCH_STM32
