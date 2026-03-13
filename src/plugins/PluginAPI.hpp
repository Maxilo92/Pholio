#pragma once

#include <cstdint>

constexpr int PHOLIO_PLUGIN_API_VERSION = 2;

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
using PholioPluginVersionFn = const char* (*)();
using PholioPluginAuthorFn = const char* (*)();
using PholioPluginProcessFn = bool (*)(const PholioPluginTask*, PholioPluginDecision*);

struct PholioPluginUiApi {
    bool (*beginWindow)(const char* title, bool* open);
    void (*endWindow)();
    void (*text)(const char* text);
    void (*textWrapped)(const char* text);
    void (*separator)();
    bool (*button)(const char* label);
};

using PholioPluginRenderWindowFn = void (*)(const PholioPluginUiApi*, bool* open);
