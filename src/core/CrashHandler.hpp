#pragma once

#include <filesystem>
#include <string>

namespace core {

class CrashHandler {
public:
    static void init(const std::filesystem::path& crashDir);
    
    /**
     * @brief Writes a crash report manually (e.g. for uncaught exceptions)
     */
    static void writeCrashReport(const std::string& reason);

private:
    static std::filesystem::path m_crashDir;
    static void handleSignal(int sig);
};

} // namespace core
