#include "../PluginAPI.hpp"
#include <cstdio>
#include <cstring>

extern "C" int pholio_plugin_api_version() {
    return PHOLIO_PLUGIN_API_VERSION;
}

extern "C" const char* pholio_plugin_name() {
    return "Skip Small Files (< 1MB)";
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

    if (task->fileSize < (1024ULL * 1024ULL)) {
        decision->skipFile = 1;
        std::snprintf(decision->skipReason, sizeof(decision->skipReason),
                      "File is smaller than 1MB");
    }
    return true;
}
