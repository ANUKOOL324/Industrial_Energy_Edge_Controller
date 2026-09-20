#include "ota_manager.h"
#include "config.h"
#include <cctype>
#include <cstring>
#include <cstdlib>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#endif

namespace {
bool extractJsonString(const char *json, size_t length, const char *key, char *out, size_t capacity) {
    const size_t keyLength = strlen(key);
    const char *end = json + length;
    for (const char *cursor = json; cursor + keyLength + 3 < end; ++cursor) {
        if (*cursor != '"' || strncmp(cursor + 1, key, keyLength) != 0 || cursor[1 + keyLength] != '"') continue;
        const char *value = cursor + keyLength + 2;
        while (value < end && (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n')) ++value;
        if (value >= end || *value++ != ':') return false;
        while (value < end && (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n')) ++value;
        if (value >= end || *value++ != '"') return false;
        const char *finish = value;
        while (finish < end && *finish != '"') ++finish;
        if (finish >= end || static_cast<size_t>(finish - value) >= capacity) return false;
        memcpy(out, value, static_cast<size_t>(finish - value));
        out[finish - value] = '\0';
        return true;
    }
    return false;
}

bool extractJsonUint(const char *json, size_t length, const char *key, uint32_t &value) {
    const size_t keyLength = strlen(key);
    const char *end = json + length;
    for (const char *cursor = json; cursor + keyLength + 3 < end; ++cursor) {
        if (*cursor != '"' || strncmp(cursor + 1, key, keyLength) != 0 || cursor[1 + keyLength] != '"') continue;
        const char *valueCursor = cursor + keyLength + 2;
        while (valueCursor < end && (*valueCursor == ' ' || *valueCursor == '\t' || *valueCursor == '\r' || *valueCursor == '\n')) ++valueCursor;
        if (valueCursor >= end || *valueCursor++ != ':') return false;
        while (valueCursor < end && (*valueCursor == ' ' || *valueCursor == '\t' || *valueCursor == '\r' || *valueCursor == '\n')) ++valueCursor;
        const bool quoted = valueCursor < end && *valueCursor == '"';
        if (quoted) ++valueCursor;
        const char *numberStart = valueCursor;
        uint64_t parsed = 0;
        while (valueCursor < end && *valueCursor >= '0' && *valueCursor <= '9') {
            parsed = parsed * 10U + static_cast<uint64_t>(*valueCursor - '0');
            if (parsed > 0xffffffffULL) return false;
            ++valueCursor;
        }
        if (valueCursor == numberStart || (quoted && (valueCursor >= end || *valueCursor != '"'))) return false;
        value = static_cast<uint32_t>(parsed);
        return true;
    }
    return false;
}

bool validSha256(const char *hash) {
    if (strlen(hash) != 64) return false;
    for (size_t i = 0; i < 64; ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(hash[i]))) return false;
    }
    return true;
}

bool sameHash(const char *left, const char *right) {
    for (size_t i = 0; i < 64; ++i) {
        if (static_cast<char>(std::tolower(static_cast<unsigned char>(left[i]))) !=
            static_cast<char>(std::tolower(static_cast<unsigned char>(right[i])))) return false;
    }
    return true;
}
}

bool parseFirmwareManifest(const char *json, size_t length, FirmwareManifest &manifest, OtaError &error) {
    error = OtaError::MANIFEST_ERROR;
    if (json == nullptr || length == 0) return false;
    FirmwareManifest parsed;
    if (!extractJsonString(json, length, "version", parsed.versionText, sizeof(parsed.versionText)) ||
        !parseSemanticVersion(parsed.versionText, parsed.version) ||
        !extractJsonString(json, length, "firmware_url", parsed.firmwareUrl, sizeof(parsed.firmwareUrl)) ||
        !extractJsonString(json, length, "sha256", parsed.sha256, sizeof(parsed.sha256)) ||
        !validSha256(parsed.sha256)) {
        error = OtaError::VERSION_INVALID;
        return false;
    }
    if (strncmp(parsed.firmwareUrl, "https://", 8) != 0 && strncmp(parsed.firmwareUrl, "http://", 7) != 0) {
        error = OtaError::MANIFEST_ERROR;
        return false;
    }
    uint32_t sizeBytes = 0;
    if (extractJsonUint(json, length, "size_bytes", sizeBytes)) parsed.sizeBytes = sizeBytes;
    manifest = parsed;
    error = OtaError::NONE;
    return true;
}

void OtaManager::begin() {
#ifndef UNIT_TEST
    // ESP-IDF marks an app as pending verification after an OTA reboot. The
    // health window below is the only point where this manager accepts it.
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t imageState = ESP_OTA_IMG_VALID;
    rollbackPending_ = running != nullptr &&
                       esp_ota_get_state_partition(running, &imageState) == ESP_OK &&
                       imageState == ESP_OTA_IMG_PENDING_VERIFY;
#endif
    state_ = OtaState::IDLE;
    error_ = OtaError::NONE;
}

void OtaManager::requestCheck() {
    if (state_ == OtaState::IDLE || state_ == OtaState::SUCCESS || state_ == OtaState::FAILED) {
        checkRequested_ = true;
        state_ = OtaState::CHECKING;
        error_ = OtaError::NONE;
    }
}

void OtaManager::reportHealth(bool energyTaskRunning, bool storageReady, bool criticalFault, size_t freeHeap) {
    energyTaskRunning_ = energyTaskRunning;
    storageReady_ = storageReady;
    criticalFault_ = criticalFault;
    freeHeap_ = freeHeap;
    healthReported_ = true;
}

void OtaManager::fail(OtaError error) {
    state_ = OtaState::FAILED;
    error_ = error;
    ++failureCount_;
}

#ifndef UNIT_TEST
bool OtaManager::fetchManifest(FirmwareManifest &manifest) {
    if (WiFi.status() != WL_CONNECTED) {
        fail(OtaError::NETWORK_UNAVAILABLE);
        return false;
    }
    HTTPClient http;
    if (!http.begin(config::otaManifestUrl)) {
        fail(OtaError::NETWORK_UNAVAILABLE);
        return false;
    }
    const int result = http.GET();
    if (result != HTTP_CODE_OK || http.getSize() <= 0 || http.getSize() > 1024) {
        http.end();
        fail(OtaError::DOWNLOAD_FAILED);
        return false;
    }
    String body = http.getString();
    http.end();
    OtaError parseError = OtaError::NONE;
    if (!parseFirmwareManifest(body.c_str(), body.length(), manifest, parseError)) {
        fail(parseError);
        return false;
    }
    return true;
}

bool OtaManager::installFirmware(const FirmwareManifest &manifest) {
    state_ = OtaState::DOWNLOADING;
    HTTPClient http;
    if (!http.begin(manifest.firmwareUrl)) {
        fail(OtaError::NETWORK_UNAVAILABLE);
        return false;
    }
    const int result = http.GET();
    const int contentLength = http.getSize();
    if (result != HTTP_CODE_OK || contentLength <= 0 ||
        (manifest.sizeBytes != 0 && static_cast<uint32_t>(contentLength) != manifest.sizeBytes) ||
        !Update.begin(static_cast<size_t>(contentLength))) {
        http.end();
        fail(OtaError::DOWNLOAD_FAILED);
        return false;
    }

    state_ = OtaState::VALIDATING;
    mbedtls_sha256_context hashContext;
    mbedtls_sha256_init(&hashContext);
    mbedtls_sha256_starts_ret(&hashContext, 0);
    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[1024];
    int written = 0;
    bool failed = false;
    while (http.connected() && written < contentLength) {
        const size_t available = stream->available();
        if (available == 0) {
            delay(1);
            continue;
        }
        const size_t requested = available > sizeof(buffer) ? sizeof(buffer) : available;
        const size_t received = stream->readBytes(buffer, requested);
        if (received == 0 || Update.write(buffer, received) != received) {
            failed = true;
            break;
        }
        mbedtls_sha256_update_ret(&hashContext, buffer, received);
        written += static_cast<int>(received);
    }
    uint8_t digest[32];
    mbedtls_sha256_finish_ret(&hashContext, digest);
    mbedtls_sha256_free(&hashContext);
    char digestText[65] = {};
    for (size_t i = 0; i < 32; ++i) snprintf(digestText + i * 2, 3, "%02x", digest[i]);
    http.end();
    if (failed || written != contentLength) {
        Update.abort();
        fail(OtaError::DOWNLOAD_FAILED);
        return false;
    }
    if (!sameHash(digestText, manifest.sha256)) {
        Update.abort();
        fail(OtaError::HASH_MISMATCH);
        return false;
    }
    state_ = OtaState::INSTALLING;
    if (!Update.end(true)) {
        fail(OtaError::INSTALL_FAILED);
        return false;
    }
    ++successCount_;
    state_ = OtaState::SUCCESS;
    return true;
}
#endif

void OtaManager::process(uint32_t nowMs) {
#ifndef UNIT_TEST
    if (rollbackPending_) {
        const bool healthy = healthReported_ && energyTaskRunning_ && storageReady_ &&
                             !criticalFault_ && freeHeap_ >= config::otaMinimumFreeHeap;
        static uint32_t healthySinceMs = 0;
        if (healthy && healthySinceMs == 0) healthySinceMs = nowMs;
        if (healthy && nowMs - healthySinceMs >= config::otaHealthyPeriodMs) {
            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) rollbackPending_ = false;
            else fail(OtaError::ROLLBACK_ERROR);
        } else if (!healthy) {
            healthySinceMs = 0;
        }
    }
#endif

    if (state_ == OtaState::IDLE && config::otaManifestUrl[0] != '\0' &&
        nowMs - lastAttemptMs_ >= config::otaCheckPeriodMs) {
        requestCheck();
    }
    if (state_ == OtaState::CHECKING) {
        checkRequested_ = false;
        lastAttemptMs_ = nowMs;
        ++attemptCount_;
#ifndef UNIT_TEST
        if (!fetchManifest(manifest_)) return;
#else
        return;
#endif
        SemanticVersion current{FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH};
        if (compareSemanticVersions(manifest_.version, current) <= 0) {
            state_ = OtaState::IDLE;
            return;
        }
        state_ = OtaState::UPDATE_AVAILABLE;
    }
    if (state_ == OtaState::UPDATE_AVAILABLE) {
#ifndef UNIT_TEST
        installFirmware(manifest_);
#else
        fail(OtaError::DOWNLOAD_FAILED);
#endif
    }
}
