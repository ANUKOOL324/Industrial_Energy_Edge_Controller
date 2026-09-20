#include "ota_types.h"

const char *otaStateName(OtaState state) {
    switch (state) {
        case OtaState::CHECKING: return "CHECKING";
        case OtaState::UPDATE_AVAILABLE: return "UPDATE_AVAILABLE";
        case OtaState::DOWNLOADING: return "DOWNLOADING";
        case OtaState::VALIDATING: return "VALIDATING";
        case OtaState::INSTALLING: return "INSTALLING";
        case OtaState::SUCCESS: return "SUCCESS";
        case OtaState::FAILED: return "FAILED";
        default: return "IDLE";
    }
}

const char *otaErrorName(OtaError error) {
    switch (error) {
        case OtaError::NETWORK_UNAVAILABLE: return "NETWORK_UNAVAILABLE";
        case OtaError::MANIFEST_ERROR: return "MANIFEST_ERROR";
        case OtaError::VERSION_INVALID: return "VERSION_INVALID";
        case OtaError::DOWNLOAD_FAILED: return "DOWNLOAD_FAILED";
        case OtaError::HASH_MISMATCH: return "HASH_MISMATCH";
        case OtaError::INSTALL_FAILED: return "INSTALL_FAILED";
        case OtaError::ROLLBACK_ERROR: return "ROLLBACK_ERROR";
        default: return "NONE";
    }
}

