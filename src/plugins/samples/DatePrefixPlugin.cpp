#include "../PluginAPI.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

extern "C" int pholio_plugin_api_version() {
    return PHOLIO_PLUGIN_API_VERSION;
}

extern "C" const char* pholio_plugin_name() {
    return "Date Prefix Target Override";
}

extern "C" const char* pholio_plugin_version() {
    return "1.0.0";
}

extern "C" const char* pholio_plugin_author() {
    return "Pholio Team";
}

extern "C" bool pholio_plugin_process(const PholioPluginTask* task, PholioPluginDecision* decision) {
    if (!task || !decision || !task->suggestedRelativeTarget) {
        return false;
    }

    std::tm tmBuf{};
    std::time_t t = static_cast<std::time_t>(task->creationUnixSeconds);
#ifdef _WIN32
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif

    char datePrefix[32] = {};
    std::strftime(datePrefix, sizeof(datePrefix), "%Y-%m-%d", &tmBuf);

    std::string overridden = std::string(datePrefix) + "/" + task->suggestedRelativeTarget;
    std::snprintf(decision->overrideRelativeTarget, sizeof(decision->overrideRelativeTarget),
                  "%s", overridden.c_str());
    return true;
}
