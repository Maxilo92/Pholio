#include "PluginManager.hpp"
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace plugins {

namespace {
void* loadLibrary(const std::filesystem::path& path) {
#ifdef _WIN32
    return reinterpret_cast<void*>(LoadLibraryA(path.string().c_str()));
#else
    return dlopen(path.string().c_str(), RTLD_NOW);
#endif
}

void* loadSymbol(void* handle, const char* symbol) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol));
#else
    return dlsym(handle, symbol);
#endif
}

void closeLibrary(void* handle) {
#ifdef _WIN32
    if (handle) {
        FreeLibrary(reinterpret_cast<HMODULE>(handle));
    }
#else
    if (handle) {
        dlclose(handle);
    }
#endif
}
} // namespace

PluginManager::PluginManager(ui::LogWindow& logWindow)
    : core::Loggable(logWindow) {}

PluginManager::~PluginManager() {
    unloadAll();
}

void PluginManager::loadFromDirectory(const std::filesystem::path& directory) {
    unloadAll();

    if (directory.empty()) {
        warn("PluginManager: plugin directory is empty.");
        return;
    }
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        warn("PluginManager: plugin directory not found: " + directory.string());
        return;
    }

    int loadedCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file() || !hasSupportedExtension(entry.path())) {
            continue;
        }

        void* handle = loadLibrary(entry.path());
        if (!handle) {
            warn("PluginManager: failed to load plugin library: " + entry.path().string());
            continue;
        }

        auto* apiVersion = reinterpret_cast<PholioPluginApiVersionFn>(loadSymbol(handle, "pholio_plugin_api_version"));
        auto* nameFn = reinterpret_cast<PholioPluginNameFn>(loadSymbol(handle, "pholio_plugin_name"));
        auto* processFn = reinterpret_cast<PholioPluginProcessFn>(loadSymbol(handle, "pholio_plugin_process"));

        if (!apiVersion || !nameFn || !processFn) {
            warn("PluginManager: plugin missing required symbols: " + entry.path().filename().string());
            closeLibrary(handle);
            continue;
        }

        if (apiVersion() != PHOLIO_PLUGIN_API_VERSION) {
            warn("PluginManager: plugin API mismatch for " + entry.path().filename().string());
            closeLibrary(handle);
            continue;
        }

        LoadedPlugin plugin;
        plugin.path = entry.path();
        plugin.name = nameFn() ? std::string(nameFn()) : entry.path().stem().string();
        plugin.handle = handle;
        plugin.process = processFn;
        m_plugins.push_back(std::move(plugin));
        loadedCount++;
    }

    info("PluginManager: loaded " + std::to_string(loadedCount) + " plugin(s).");
}

bool PluginManager::apply(engine::MediaTask& task,
                          const std::filesystem::path& targetRoot,
                          std::filesystem::path& inOutTargetPath,
                          std::string& skipReason) {
    if (m_plugins.empty()) {
        return true;
    }

    std::error_code relEc;
    std::filesystem::path relativeTarget = std::filesystem::relative(inOutTargetPath, targetRoot, relEc);
    if (relEc) {
        relativeTarget = inOutTargetPath.filename();
    }

    const std::string sourcePath = task.metadata.path.string();
    const std::string sourceFilename = task.metadata.path.filename().string();
    const std::string sourceExtension = task.metadata.path.extension().string();
    const std::string suggestedRelativeTarget = relativeTarget.generic_string();
    const auto creationUnix = std::chrono::system_clock::to_time_t(task.metadata.creationTime);

    PholioPluginTask pluginTask{};
    pluginTask.sourcePath = sourcePath.c_str();
    pluginTask.sourceFilename = sourceFilename.c_str();
    pluginTask.sourceExtension = sourceExtension.c_str();
    pluginTask.suggestedRelativeTarget = suggestedRelativeTarget.c_str();
    pluginTask.creationUnixSeconds = static_cast<int64_t>(creationUnix);
    pluginTask.fileSize = task.metadata.fileSize;
    if (task.metadata.type == engine::MediaType::Image) {
        pluginTask.mediaType = PholioPluginMediaType::Image;
    } else if (task.metadata.type == engine::MediaType::Video) {
        pluginTask.mediaType = PholioPluginMediaType::Video;
    } else {
        pluginTask.mediaType = PholioPluginMediaType::Unknown;
    }

    for (const auto& plugin : m_plugins) {
        PholioPluginDecision decision{};
        decision.skipFile = 0;
        decision.skipReason[0] = '\0';
        decision.overrideRelativeTarget[0] = '\0';

        const bool ok = plugin.process(&pluginTask, &decision);
        if (!ok) {
            warn("PluginManager: plugin execution failed: " + plugin.name);
            continue;
        }

        if (decision.skipFile != 0) {
            skipReason = std::strlen(decision.skipReason) > 0
                ? std::string(decision.skipReason)
                : ("Plugin '" + plugin.name + "' requested skip");
            return false;
        }

        if (std::strlen(decision.overrideRelativeTarget) > 0) {
            std::filesystem::path overrideRelativePath(decision.overrideRelativeTarget);
            if (!isSafeRelativePath(overrideRelativePath)) {
                warn("PluginManager: ignored unsafe override path from plugin " + plugin.name);
                continue;
            }
            inOutTargetPath = targetRoot / overrideRelativePath;
        }
    }

    return true;
}

bool PluginManager::hasPlugins() const {
    return !m_plugins.empty();
}

bool PluginManager::hasSupportedExtension(const std::filesystem::path& path) {
#ifdef _WIN32
    return path.extension() == ".dll";
#elif __APPLE__
    return path.extension() == ".dylib";
#else
    return path.extension() == ".so";
#endif
}

bool PluginManager::isSafeRelativePath(const std::filesystem::path& relativePath) {
    if (relativePath.empty() || relativePath.is_absolute()) {
        return false;
    }
    const auto normalized = relativePath.lexically_normal();
    for (const auto& part : normalized) {
        if (part == "..") {
            return false;
        }
    }
    return true;
}

void PluginManager::unloadAll() {
    for (auto& plugin : m_plugins) {
        closeLibrary(plugin.handle);
        plugin.handle = nullptr;
    }
    m_plugins.clear();
}

} // namespace plugins
