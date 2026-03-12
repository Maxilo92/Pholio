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

// We use a global to keep backward alive if needed
static backward::SignalHandling sh;

void CrashHandler::init(const std::filesystem::path& crashDir) {
    m_crashDir = crashDir;
    if (!std::filesystem::exists(m_crashDir)) {
        std::filesystem::create_directories(m_crashDir);
    }
}

void CrashHandler::writeCrashReport(const std::string& reason) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss_folder;
    ss_folder << "crash_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    
    std::filesystem::path reportDir = m_crashDir / ss_folder.str();
    std::filesystem::create_directories(reportDir);
    
    std::filesystem::path reportPath = reportDir / "report.txt";
    std::ofstream file(reportPath);
    
    if (file.is_open()) {
        file << "========================================" << std::endl;
        file << "Pholio CRASH REPORT" << std::endl;
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
