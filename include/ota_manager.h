#pragma once

#include <stddef.h>
#include <stdint.h>
#include "ota_types.h"
#include "version.h"

struct FirmwareManifest {
    SemanticVersion version;
    char versionText[16] = {};
    char firmwareUrl[192] = {};
    char sha256[65] = {};
    uint32_t sizeBytes = 0;
};

bool parseFirmwareManifest(const char *json, size_t length, FirmwareManifest &manifest, OtaError &error);

class OtaManager {
public:
    void begin();
    void requestCheck();
    void process(uint32_t nowMs);
    void reportHealth(bool energyTaskRunning, bool storageReady, bool criticalFault, size_t freeHeap);

    OtaState state() const { return state_; }
    OtaError error() const { return error_; }
    const char *targetVersion() const { return manifest_.versionText; }
    uint32_t attemptCount() const { return attemptCount_; }
    uint32_t successCount() const { return successCount_; }
    uint32_t failureCount() const { return failureCount_; }
    uint32_t lastAttemptMs() const { return lastAttemptMs_; }
    bool rebootRequired() const { return state_ == OtaState::SUCCESS; }

private:
    void fail(OtaError error);
#ifndef UNIT_TEST
    bool fetchManifest(FirmwareManifest &manifest);
    bool installFirmware(const FirmwareManifest &manifest);
    bool rollbackPending_ = false;
#endif
    OtaState state_ = OtaState::IDLE;
    OtaError error_ = OtaError::NONE;
    FirmwareManifest manifest_;
    bool checkRequested_ = false;
    bool healthReported_ = false;
    uint32_t attemptCount_ = 0;
    uint32_t successCount_ = 0;
    uint32_t failureCount_ = 0;
    uint32_t lastAttemptMs_ = 0;
    bool energyTaskRunning_ = false;
    bool storageReady_ = false;
    bool criticalFault_ = false;
    size_t freeHeap_ = 0;
};
