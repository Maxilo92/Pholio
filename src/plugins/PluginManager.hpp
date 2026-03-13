#pragma once

#include "PluginAPI.hpp"
#include "../core/Loggable.hpp"
#include "../engine/Types.hpp"
#include <filesystem>
#include <unordered_set>
#include <vector>

namespace plugins {

class PluginManager : public core::Loggable {
public:
    struct PluginWindowState {
        std::string name;
        bool enabled = true;
        bool open = false;
    };
    struct PluginDescriptor {
        std::string name;
        std::string version;
        std::string author;
        bool enabled = true;
        bool hasWindow = false;
    };

    explicit PluginManager(ui::LogWindow& logWindow);
    ~PluginManager();

    void loadFromDirectory(const std::filesystem::path& directory);
    bool apply(engine::MediaTask& task,
               const std::filesystem::path& targetRoot,
               std::filesystem::path& inOutTargetPath,
               std::string& skipReason);
    void renderWindows();
    std::vector<PluginWindowState> getPluginWindowStates() const;
    bool reopenPluginWindow(const std::string& pluginName);
    std::vector<PluginDescriptor> getPluginDescriptors() const;
    bool activatePlugin(const std::string& pluginName, std::string& message);
    bool setPluginEnabled(const std::string& pluginName, bool enabled);
    bool isPluginEnabled(const std::string& pluginName) const;
    void setDisabledPlugins(const std::vector<std::string>& disabledPlugins);
    std::vector<std::string> getDisabledPlugins() const;
    bool hasPlugins() const;

private:
#ifdef _WIN32
    using PluginHandle = void*;
#else
    using PluginHandle = void*;
#endif

    struct LoadedPlugin {
        std::filesystem::path path;
        std::string name;
        std::string version;
        std::string author;
        PluginHandle handle = nullptr;
        PholioPluginProcessFn process = nullptr;
        PholioPluginRenderWindowFn renderWindow = nullptr;
        bool enabled = true;
        bool windowOpen = true;
    };

    std::vector<LoadedPlugin> m_plugins;
    std::unordered_set<std::string> m_disabledPluginNames;

    static bool hasSupportedExtension(const std::filesystem::path& path);
    static bool isSafeRelativePath(const std::filesystem::path& relativePath);
    void unloadAll();
};

} // namespace plugins
