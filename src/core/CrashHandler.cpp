#include "CrashHandler.hpp"
#include "core/Config.hpp"
#include <backward.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <csignal>

namespace core {

std::filesystem::path CrashHandler::m_crashDir;

// We use a global to keep backward alive if needed, 
// but backward::SignalHandling handles its own registration.
static backward::SignalHandling sh;

void CrashHandler::init(const std::filesystem::path& crashDir) {
    m_crashDir = crashDir;
    if (!std::filesystem::exists(m_crashDir)) {
        std::filesystem::create_directories(m_crashDir);
    }

    // Register our own fallback for some signals if needed, 
    // but backward handles most.
    // For specific extra info, we can write a report on exit if we detect a crash.
    std::cout << "CrashHandler initialized. Reports will be saved to: " << m_crashDir.string() << std::endl;
}

void CrashHandler::writeCrashReport(const std::string& reason) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "crash_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".txt";
    
    std::filesystem::path reportPath = m_crashDir / ss.str();
    std::ofstream file(reportPath);
    
    if (file.is_open()) {
        file << "========================================" << std::endl;
        file << "VerwaltungV5 CRASH REPORT" << std::endl;
        file << "========================================" << std::endl;
        file << "Version: " << core::PROJECT_VERSION << std::endl;
        file << "Timestamp: " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
        file << "Reason: " << reason << std::endl;
        file << "----------------------------------------" << std::endl;
        file << "Stack Trace:" << std::endl;
        
        backward::StackTrace st; 
        st.load_here(32);
        backward::Printer p;
        p.print(st, file);
        
        file << "========================================" << std::endl;
        file.close();
        
        std::cerr << "CRITICAL ERROR: Application crashed. Report saved to " << reportPath.string() << std::endl;
    }
}

} // namespace core
