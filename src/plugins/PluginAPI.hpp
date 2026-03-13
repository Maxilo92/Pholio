#pragma once

#include <cstdint>

constexpr int PHOLIO_PLUGIN_API_VERSION = 1;

enum class PholioPluginMediaType : int {
    Unknown = 0,
    Image = 1,
    Video = 2
};

struct PholioPluginTask {
    const char* sourcePath;
    const char* sourceFilename;
    const char* sourceExtension;
    const char* suggestedRelativeTarget;
    int64_t creationUnixSeconds;
    uint64_t fileSize;
    PholioPluginMediaType mediaType;
};

struct PholioPluginDecision {
    int skipFile;
    char skipReason[256];
    char overrideRelativeTarget[1024];
};

using PholioPluginApiVersionFn = int (*)();
using PholioPluginNameFn = const char* (*)();
using PholioPluginProcessFn = bool (*)(const PholioPluginTask*, PholioPluginDecision*);
