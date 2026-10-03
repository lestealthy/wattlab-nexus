#include "nexus_storage_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <sys/stat.h>

/*
 * Host storage backend. Uses the C library for portability across the
 * supported desktop toolchains (MinGW/MSVC).
 */

#define HOST_ROOT_MAX 512
#define HOST_PATH_MAX 768
#define HOST_MAX_OPEN 32

static char s_root[HOST_ROOT_MAX] = ".";
static bool s_mounted = false;
static bool s_inject_full = false;
static bool s_inject_write_failure = false;
static bool s_inject_mount_failure = false;

typedef struct {
    bool used;
    FILE* fp;
    uint32_t mode;
    uint32_t size;
} host_handle_t;

static host_handle_t s_handles[HOST_MAX_OPEN];

// Map a logical path to an absolute host path.
static bool map_path(const char* logical, char* out, size_t out_size) {
    if (!logical || logical[0] != '/') return false;
    int n = snprintf(out, out_size, "%s%s", s_root, logical);
    return (n > 0 && (size_t)n < out_size);
}

static void ensure_parent_dirs(const char* path) {
    // Create intermediate directories for `path` (starting after s_root).
    char buf[HOST_PATH_MAX];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    for (char* p = buf + 1; *p; p++) {
        if (*p == '/') {
            char saved = *p;
            *p = '\0';
            _mkdir(buf);
            *p = saved;
        }
    }
}

static bool host_exists(const char* path) {
    char full[HOST_PATH_MAX];
    if (!map_path(path, full, sizeof(full))) return false;
    struct _stat st;
    return _stat(full, &st) == 0;
}

static nexus_err_t host_mkdir(const char* path) {
    char full[HOST_PATH_MAX];
    if (!map_path(path, full, sizeof(full))) return NEXUS_ERR_INVALID_PARAM;
    ensure_parent_dirs(full);
    if (_mkdir(full) == 0) return NEXUS_OK;
    // Already exists is fine.
    struct _stat st;
    if (_stat(full, &st) == 0 && (st.st_mode & _S_IFDIR)) return NEXUS_OK;
    return NEXUS_ERR_STORAGE;
}

static nexus_err_t host_remove(const char* path) {
    char full[HOST_PATH_MAX];
    if (!map_path(path, full, sizeof(full))) return NEXUS_ERR_INVALID_PARAM;
    if (remove(full) != 0) {
        if (_rmdir(full) != 0) return NEXUS_ERR_NOT_FOUND;
    }
    return NEXUS_OK;
}

static nexus_err_t host_rename(const char* from, const char* to) {
    char a[HOST_PATH_MAX], b[HOST_PATH_MAX];
    if (!map_path(from, a, sizeof(a)) || !map_path(to, b, sizeof(b))) return NEXUS_ERR_INVALID_PARAM;
    // rename() does not overwrite on Windows; remove target first.
    remove(b);
    return (rename(a, b) == 0) ? NEXUS_OK : NEXUS_ERR_STORAGE;
}

static nexus_file_t* host_open(const char* path, uint32_t mode) {
    if (s_inject_write_failure && (mode & (NEXUS_FILE_WRITE | NEXUS_FILE_APPEND))) {
        return NULL;
    }
    char full[HOST_PATH_MAX];
    if (!map_path(path, full, sizeof(full))) return NULL;

    const char* cmode = "rb";
    if ((mode & NEXUS_FILE_WRITE)) {
        ensure_parent_dirs(full);
        cmode = "wb";
    } else if (mode & NEXUS_FILE_APPEND) {
        ensure_parent_dirs(full);
        cmode = "ab";
    }

    FILE* fp = fopen(full, cmode);
    if (!fp && (mode & (NEXUS_FILE_WRITE | NEXUS_FILE_APPEND))) {
        // Try creating parent dirs then retry once.
        ensure_parent_dirs(full);
        fp = fopen(full, cmode);
    }
    if (!fp) return NULL;

    if (s_inject_full && (mode & (NEXUS_FILE_WRITE | NEXUS_FILE_APPEND))) {
        // Simulate a full card by allowing open but failing the subsequent write.
    }

    // Find a free handle slot.
    for (int i = 0; i < HOST_MAX_OPEN; i++) {
        if (!s_handles[i].used) {
            s_handles[i].used = true;
            s_handles[i].fp = fp;
            s_handles[i].mode = mode;
            // Determine size.
            fseek(fp, 0, SEEK_END);
            long sz = ftell(fp);
            s_handles[i].size = (sz > 0) ? (uint32_t)sz : 0;
            if (!(mode & (NEXUS_FILE_WRITE | NEXUS_FILE_APPEND))) {
                fseek(fp, 0, SEEK_SET);
            }
            nexus_file_t* f = (nexus_file_t*)calloc(1, sizeof(nexus_file_t));
            if (!f) { fclose(fp); s_handles[i].used = false; return NULL; }
            f->handle = &s_handles[i];
            f->mode = mode;
            return f;
        }
    }
    fclose(fp);
    return NULL;
}

static host_handle_t* handle_of(nexus_file_t* f) {
    if (!f || !f->handle) return NULL;
    return (host_handle_t*)f->handle;
}

static int host_read(nexus_file_t* f, void* buf, size_t len) {
    host_handle_t* h = handle_of(f);
    if (!h || !h->fp) return -1;
    return (int)fread(buf, 1, len, h->fp);
}

static size_t host_write(nexus_file_t* f, const void* buf, size_t len) {
    host_handle_t* h = handle_of(f);
    if (!h || !h->fp) return 0;
    if (s_inject_write_failure || s_inject_full) return 0; // simulated failure
    size_t w = fwrite(buf, 1, len, h->fp);
    if (w > 0) h->size += (uint32_t)w;
    return w;
}

static bool host_seek(nexus_file_t* f, uint32_t pos) {
    host_handle_t* h = handle_of(f);
    if (!h || !h->fp) return false;
    return fseek(h->fp, (long)pos, SEEK_SET) == 0;
}

static uint32_t host_tell(nexus_file_t* f) {
    host_handle_t* h = handle_of(f);
    if (!h || !h->fp) return 0;
    long p = ftell(h->fp);
    return (p > 0) ? (uint32_t)p : 0;
}

static uint32_t host_size(nexus_file_t* f) {
    host_handle_t* h = handle_of(f);
    if (!h) return 0;
    return h->size;
}

static void host_flush(nexus_file_t* f) {
    host_handle_t* h = handle_of(f);
    if (h && h->fp) fflush(h->fp);
}

static void host_close(nexus_file_t* f) {
    host_handle_t* h = handle_of(f);
    if (h) {
        if (h->fp) fclose(h->fp);
        h->used = false;
        h->fp = NULL;
    }
    free(f);
}

static nexus_err_t host_mount(void) {
    if (s_inject_mount_failure) return NEXUS_ERR_STORAGE;
    // Create the root if needed.
    _mkdir(s_root);
    struct _stat st;
    if (_stat(s_root, &st) != 0) return NEXUS_ERR_STORAGE;
    s_mounted = true;
    return NEXUS_OK;
}

static nexus_err_t host_unmount(void) {
    s_mounted = false;
    return NEXUS_OK;
}

static nexus_storage_state_t host_state(void) {
    return s_mounted ? NEXUS_STORAGE_READY : NEXUS_STORAGE_NOT_PRESENT;
}

static nexus_err_t host_info(nexus_storage_info_t* out) {
    if (!out) return NEXUS_ERR_INVALID_PARAM;
    memset(out, 0, sizeof(*out));
    out->mounted = s_mounted;
    // Report a synthetic-but-stable capacity for the host backend.
    out->total_bytes = 32ULL * 1024 * 1024 * 1024;
    out->free_bytes = 24ULL * 1024 * 1024 * 1024;
    out->used_bytes = out->total_bytes - out->free_bytes;
    out->fat_type = 32;
    strncpy(out->label, "HOSTFS", sizeof(out->label) - 1);
    return NEXUS_OK;
}

static nexus_err_t host_list(const char* dir, nexus_file_entry_t* out, uint32_t max, uint32_t* count) {
    if (!out || !count) return NEXUS_ERR_INVALID_PARAM;
    *count = 0;
    char full[HOST_PATH_MAX];
    if (!map_path(dir, full, sizeof(full))) return NEXUS_ERR_INVALID_PARAM;

    char pattern[HOST_PATH_MAX + 4];
    int pn = snprintf(pattern, sizeof(pattern), "%s/*", full);
    if (pn < 0 || (size_t)pn >= sizeof(pattern)) return NEXUS_ERR_INVALID_PARAM;

    // Use _findfirst/_findnext via the Windows CRT.
    struct _finddata_t fd;
    intptr_t h = _findfirst(pattern, &fd);
    if (h == -1) return NEXUS_OK; // empty dir

    do {
        if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
        if (*count >= max) break;
        nexus_file_entry_t* e = &out[*count];
        memset(e, 0, sizeof(*e));
        strncpy(e->name, fd.name, sizeof(e->name) - 1);
        e->is_directory = (fd.attrib & _A_SUBDIR) != 0;
        e->size = (uint32_t)fd.size;
        e->modified = (uint32_t)fd.time_write;
        (*count)++;
    } while (_findnext(h, &fd) == 0);

    _findclose(h);
    return NEXUS_OK;
}

static nexus_storage_backend_t s_host_backend = {
    .name = "host",
    .mount = host_mount,
    .unmount = host_unmount,
    .state = host_state,
    .info = host_info,
    .exists = host_exists,
    .mkdir = host_mkdir,
    .remove = host_remove,
    .rename = host_rename,
    .open = host_open,
    .read = host_read,
    .write = host_write,
    .seek = host_seek,
    .tell = host_tell,
    .size = host_size,
    .flush = host_flush,
    .close = host_close,
    .list = host_list,
};

void nexus_storage_host_set_root(const char* root_dir) {
    if (!root_dir) return;
    strncpy(s_root, root_dir, sizeof(s_root) - 1);
    s_root[sizeof(s_root) - 1] = '\0';
    // Normalise trailing slash.
    size_t n = strlen(s_root);
    while (n > 1 && (s_root[n - 1] == '/' || s_root[n - 1] == '\\')) {
        s_root[--n] = '\0';
    }
}

nexus_storage_backend_t* nexus_storage_host_backend(void) {
    return &s_host_backend;
}

void nexus_storage_host_inject_full(bool full) { s_inject_full = full; }
void nexus_storage_host_inject_write_failure(bool fail) { s_inject_write_failure = fail; }
void nexus_storage_host_inject_mount_failure(bool fail) { s_inject_mount_failure = fail; }
