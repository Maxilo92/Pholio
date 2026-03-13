#pragma once

#include "PluginAPI.hpp"
#include "../core/Loggable.hpp"
#include "../engine/Types.hpp"
#include <filesystem>
#include <vector>

namespace plugins {

class PluginManager : public core::Loggable {
public:
    explicit PluginManager(ui::LogWindow& logWindow);
    ~PluginManager();

    void loadFromDirectory(const std::filesystem::path& directory);
    bool apply(engine::MediaTask& task,
               const std::filesystem::path& targetRoot,
               std::filesystem::path& inOutTargetPath,
               std::string& skipReason);
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
        PluginHandle handle = nullptr;
        PholioPluginProcessFn process = nullptr;
    };

    std::vector<LoadedPlugin> m_plugins;

    static bool hasSupportedExtension(const std::filesystem::path& path);
    static bool isSafeRelativePath(const std::filesystem::path& relativePath);
    void unloadAll();
};

} // namespace plugins
