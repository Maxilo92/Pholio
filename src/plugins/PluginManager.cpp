#include "PluginManager.hpp"
#include <imgui.h>
#include <algorithm>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace plugins {

namespace {
bool uiBeginWindow(const char* title, bool* open) {
    return ImGui::Begin(title ? title : "Plugin Window", open);
}

void uiEndWindow() {
    ImGui::End();
}

void uiText(const char* text) {
    ImGui::Text("%s", text ? text : "");
}

void uiTextWrapped(const char* text) {
    ImGui::TextWrapped("%s", text ? text : "");
}

void uiSeparator() {
    ImGui::Separator();
}

bool uiButton(const char* label) {
    return ImGui::Button(label ? label : "Button");
}

const PholioPluginUiApi kUiApi = {
    &uiBeginWindow,
    &uiEndWindow,
    &uiText,
    &uiTextWrapped,
    &uiSeparator,
    &uiButton
};

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
        auto* versionFn = reinterpret_cast<PholioPluginVersionFn>(loadSymbol(handle, "pholio_plugin_version"));
        auto* authorFn = reinterpret_cast<PholioPluginAuthorFn>(loadSymbol(handle, "pholio_plugin_author"));
        auto* processFn = reinterpret_cast<PholioPluginProcessFn>(loadSymbol(handle, "pholio_plugin_process"));
        auto* renderWindowFn = reinterpret_cast<PholioPluginRenderWindowFn>(loadSymbol(handle, "pholio_plugin_render_window"));

        if (!apiVersion || !nameFn || (!processFn && !renderWindowFn)) {
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
        plugin.version = versionFn && versionFn() ? std::string(versionFn()) : "unknown";
        plugin.author = authorFn && authorFn() ? std::string(authorFn()) : "unknown";
        plugin.handle = handle;
        plugin.process = processFn;
        plugin.renderWindow = renderWindowFn;
        plugin.enabled = m_disabledPluginNames.find(plugin.name) == m_disabledPluginNames.end();
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
        if (!plugin.enabled) {
            continue;
        }
        if (!plugin.process) {
            continue;
        }
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

void PluginManager::renderWindows() {
    for (auto& plugin : m_plugins) {
        if (!plugin.enabled) {
            continue;
        }
        if (!plugin.renderWindow) {
            continue;
        }
        try {
            plugin.renderWindow(&kUiApi, &plugin.windowOpen);
        } catch (...) {
            warn("PluginManager: plugin UI rendering failed: " + plugin.name);
            plugin.windowOpen = false;
        }
    }
}

std::vector<PluginManager::PluginWindowState> PluginManager::getPluginWindowStates() const {
    std::vector<PluginWindowState> states;
    states.reserve(m_plugins.size());
    for (const auto& plugin : m_plugins) {
        if (!plugin.renderWindow) {
            continue;
        }
        states.push_back(PluginWindowState{plugin.name, plugin.enabled, plugin.windowOpen});
    }
    return states;
}

bool PluginManager::reopenPluginWindow(const std::string& pluginName) {
    for (auto& plugin : m_plugins) {
        if (!plugin.renderWindow) {
            continue;
        }
        if (plugin.name == pluginName) {
            plugin.windowOpen = true;
            return true;
        }
    }
    return false;
}

std::vector<PluginManager::PluginDescriptor> PluginManager::getPluginDescriptors() const {
    std::vector<PluginDescriptor> descriptors;
    descriptors.reserve(m_plugins.size());
    for (const auto& plugin : m_plugins) {
        descriptors.push_back(PluginDescriptor{
            plugin.name,
            plugin.version,
            plugin.author,
            plugin.enabled,
            plugin.renderWindow != nullptr
        });
    }
    return descriptors;
}

bool PluginManager::activatePlugin(const std::string& pluginName, std::string& message) {
    for (auto& plugin : m_plugins) {
        if (plugin.name != pluginName) {
            continue;
        }
        if (!plugin.enabled) {
            message = "Plugin is disabled: " + plugin.name;
            return false;
        }
        if (plugin.renderWindow) {
            plugin.windowOpen = true;
            message = "Opened plugin window: " + plugin.name;
            return true;
        }
        message = "Plugin info - Name: " + plugin.name + ", Version: " + plugin.version + ", Author: " + plugin.author;
        return false;
    }
    message = "Plugin not found: " + pluginName;
    return false;
}

bool PluginManager::hasPlugins() const {
    return !m_plugins.empty();
}

bool PluginManager::setPluginEnabled(const std::string& pluginName, bool enabled) {
    bool found = false;
    if (enabled) {
        m_disabledPluginNames.erase(pluginName);
    } else {
        m_disabledPluginNames.insert(pluginName);
    }

    for (auto& plugin : m_plugins) {
        if (plugin.name != pluginName) {
            continue;
        }
        plugin.enabled = enabled;
        if (!enabled) {
            plugin.windowOpen = false;
        }
        found = true;
    }
    return found;
}

bool PluginManager::isPluginEnabled(const std::string& pluginName) const {
    return m_disabledPluginNames.find(pluginName) == m_disabledPluginNames.end();
}

void PluginManager::setDisabledPlugins(const std::vector<std::string>& disabledPlugins) {
    m_disabledPluginNames.clear();
    for (const auto& name : disabledPlugins) {
        if (!name.empty()) {
            m_disabledPluginNames.insert(name);
        }
    }
    for (auto& plugin : m_plugins) {
        plugin.enabled = m_disabledPluginNames.find(plugin.name) == m_disabledPluginNames.end();
        if (!plugin.enabled) {
            plugin.windowOpen = false;
        }
    }
}

std::vector<std::string> PluginManager::getDisabledPlugins() const {
    std::vector<std::string> disabled(m_disabledPluginNames.begin(), m_disabledPluginNames.end());
    std::sort(disabled.begin(), disabled.end());
    return disabled;
}

bool PluginManager::hasSupportedExtension(const std::filesystem::path& path) {
#ifdef _WIN32
    return path.extension() == ".dll";
#elif __APPLE__
    return path.extension() == ".dylib" || path.extension() == ".so";
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
