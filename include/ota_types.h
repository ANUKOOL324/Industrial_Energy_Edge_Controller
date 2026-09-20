#pragma once

#include <stdint.h>

enum class OtaState : uint8_t {
    IDLE,
    CHECKING,
    UPDATE_AVAILABLE,
    DOWNLOADING,
    VALIDATING,
    INSTALLING,
    SUCCESS,
    FAILED
};

enum class OtaError : uint8_t {
    NONE,
    NETWORK_UNAVAILABLE,
    MANIFEST_ERROR,
    VERSION_INVALID,
    DOWNLOAD_FAILED,
    HASH_MISMATCH,
    INSTALL_FAILED,
    ROLLBACK_ERROR
};

const char *otaStateName(OtaState state);
const char *otaErrorName(OtaError error);

