#include "nexus_errors.h"

const char* nexus_err_string(nexus_err_t err) {
    switch (err) {
        case NEXUS_OK: return "OK";
        case NEXUS_ERR_GENERIC: return "Generic error";
        case NEXUS_ERR_TIMEOUT: return "Timeout";
        case NEXUS_ERR_INVALID_CONFIG: return "Invalid configuration";
        case NEXUS_ERR_RESOURCE_BUSY: return "Resource busy";
        case NEXUS_ERR_PROTOCOL: return "Protocol error";
        case NEXUS_ERR_CALIBRATION: return "Calibration error";
        case NEXUS_ERR_HARDWARE: return "Hardware error";
        case NEXUS_ERR_STORAGE: return "Storage error";
        case NEXUS_ERR_MEMORY: return "Memory error";
        case NEXUS_ERR_INVALID_PARAM: return "Invalid parameter";
        case NEXUS_ERR_NOT_FOUND: return "Not found";
        case NEXUS_ERR_BUFFER_OVERFLOW: return "Buffer overflow";
        case NEXUS_ERR_NOT_SUPPORTED: return "Not supported";
        case NEXUS_ERR_SAFETY: return "Safety violation";
        default: return "Unknown error";
    }
}
