#include "../PluginAPI.hpp"
#include <cstdio>
#include <cstring>

extern "C" int pholio_plugin_api_version() {
    return PHOLIO_PLUGIN_API_VERSION;
}

extern "C" const char* pholio_plugin_name() {
    return "Videos Only";
}

extern "C" const char* pholio_plugin_version() {
    return "1.0.0";
}

extern "C" const char* pholio_plugin_author() {
    return "Pholio Team";
}

extern "C" bool pholio_plugin_process(const PholioPluginTask* task, PholioPluginDecision* decision) {
    if (!task || !decision) {
        return false;
    }

    if (task->mediaType != PholioPluginMediaType::Video) {
        decision->skipFile = 1;
        std::snprintf(decision->skipReason, sizeof(decision->skipReason),
                      "Only video files are allowed by this plugin");
    }
    return true;
}
