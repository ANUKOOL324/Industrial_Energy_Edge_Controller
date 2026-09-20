#include "version.h"
#include <cstdio>
#include <cstring>

namespace {
bool parsePart(const char *&cursor, uint16_t &value) {
    if (cursor == nullptr || *cursor < '0' || *cursor > '9') return false;
    uint32_t parsed = 0;
    while (*cursor >= '0' && *cursor <= '9') {
        parsed = parsed * 10U + static_cast<uint32_t>(*cursor - '0');
        if (parsed > 65535U) return false;
        ++cursor;
    }
    value = static_cast<uint16_t>(parsed);
    return true;
}
}

const char *firmwareVersionString() {
    static char version[16];
    static bool initialized = false;
    if (!initialized) {
        snprintf(version, sizeof(version), "%u.%u.%u", FIRMWARE_VERSION_MAJOR,
                 FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
        initialized = true;
    }
    return version;
}

bool parseSemanticVersion(const char *text, SemanticVersion &version) {
    if (text == nullptr || *text == '\0') return false;
    const char *cursor = text;
    SemanticVersion parsed;
    if (!parsePart(cursor, parsed.major) || *cursor++ != '.') return false;
    if (!parsePart(cursor, parsed.minor) || *cursor++ != '.') return false;
    if (!parsePart(cursor, parsed.patch) || *cursor != '\0') return false;
    version = parsed;
    return true;
}

int compareSemanticVersions(const SemanticVersion &left, const SemanticVersion &right) {
    if (left.major != right.major) return left.major < right.major ? -1 : 1;
    if (left.minor != right.minor) return left.minor < right.minor ? -1 : 1;
    if (left.patch != right.patch) return left.patch < right.patch ? -1 : 1;
    return 0;
}

