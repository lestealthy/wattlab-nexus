#include "nexus_resource.h"
#include "nexus_log.h"
#include <string.h>

#define MAX_RESOURCES 64

static nexus_resource_t s_resources[MAX_RESOURCES];
static bool s_initialized = false;

nexus_err_t nexus_resource_init(void) {
    memset(s_resources, 0, sizeof(s_resources));
    s_initialized = true;
    NEXUS_LOG_INFO("RESOURCE", "Resource manager initialized");
    return NEXUS_OK;
}

nexus_err_t nexus_resource_acquire(uint32_t id, nexus_resource_type_t type, const char* owner, void* config) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;

    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (s_resources[i].state == NEXUS_RES_FREE) {
            s_resources[i].id = id;
            s_resources[i].type = type;
            s_resources[i].state = NEXUS_RES_ACQUIRED;
            s_resources[i].owner = owner;
            s_resources[i].config = config;
            NEXUS_LOG_DEBUG("RESOURCE", "Acquired resource %u type %d for %s", id, type, owner);
            return NEXUS_OK;
        }
    }
    return NEXUS_ERR_RESOURCE_BUSY;
}

nexus_err_t nexus_resource_release(uint32_t id, nexus_resource_type_t type, const char* owner) {
    if (!s_initialized) return NEXUS_ERR_HARDWARE;

    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (s_resources[i].id == id && s_resources[i].type == type) {
            if (s_resources[i].owner && owner && strcmp(s_resources[i].owner, owner) != 0) {
                return NEXUS_ERR_SAFETY;
            }
            s_resources[i].state = NEXUS_RES_FREE;
            s_resources[i].owner = NULL;
            s_resources[i].config = NULL;
            NEXUS_LOG_DEBUG("RESOURCE", "Released resource %u type %d", id, type);
            return NEXUS_OK;
        }
    }
    return NEXUS_ERR_NOT_FOUND;
}

nexus_err_t nexus_resource_query(uint32_t id, nexus_resource_type_t type, nexus_resource_t* out) {
    if (!s_initialized || !out) return NEXUS_ERR_INVALID_PARAM;

    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (s_resources[i].id == id && s_resources[i].type == type) {
            *out = s_resources[i];
            return NEXUS_OK;
        }
    }
    return NEXUS_ERR_NOT_FOUND;
}

bool nexus_resource_is_available(uint32_t id, nexus_resource_type_t type) {
    if (!s_initialized) return false;

    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (s_resources[i].id == id && s_resources[i].type == type) {
            return s_resources[i].state == NEXUS_RES_FREE;
        }
    }
    return true;
}

const char* nexus_resource_owner(uint32_t id, nexus_resource_type_t type) {
    if (!s_initialized) return NULL;

    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (s_resources[i].id == id && s_resources[i].type == type) {
            return s_resources[i].owner;
        }
    }
    return NULL;
}
