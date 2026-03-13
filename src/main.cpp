#include <iostream>
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <thread>
#include <cstdlib>
#include <system_error>
#include <optional>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include "core/ConfigManager.hpp"
#include "core/Config.hpp"
#include "core/CrashHandler.hpp"
#include "core/UpdateManager.hpp"
#include "ui/AppWindow.hpp"

static std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

static std::optional<std::string> getArgValue(const std::vector<std::string>& args, const std::string& name) {
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == name) {
            return args[i + 1];
        }
    }
    return std::nullopt;
}

static bool hasArg(const std::vector<std::string>& args, const std::string& name) {
    for (const auto& a : args) {
        if (a == name) return true;
    }
    return false;
}

static int runApplyUpdateCli(const std::vector<std::string>& args) {
    auto zipArg = getArgValue(args, "--zip");
    auto targetArg = getArgValue(args, "--target-app");
    auto waitPidArg = getArgValue(args, "--wait-pid");
    bool relaunch = hasArg(args, "--relaunch");

    std::filesystem::path logPath = std::filesystem::temp_directory_path() / "pholio_apply_update.log";
    std::ofstream log(logPath, std::ios::app);
    auto logLine = [&](const std::string& msg) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char timestamp[64];
        std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", std::localtime(&now));
        if (log.is_open()) {
            log << "[" << timestamp << "] " << msg << std::endl;
            log.flush();
        }
    };

    logLine("--- apply-update invoked ---");

    if (!zipArg || !targetArg) {
        logLine("ERROR: Missing required args --zip or --target-app");
        return 1;
    }

    std::filesystem::path zipPath = *zipArg;
    std::filesystem::path targetApp = *targetArg;
    std::filesystem::path backupApp = targetApp.string() + ".bak";
    std::filesystem::path extractRoot = std::filesystem::temp_directory_path() / ("pholio_apply_" + std::to_string(getpid()));

    logLine("zip: " + zipPath.string());
    logLine("target: " + targetApp.string());

    if (waitPidArg) {
        pid_t waitPid = static_cast<pid_t>(std::stoi(*waitPidArg));
        logLine("Waiting for pid " + std::to_string(waitPid));
        for (int i = 0; i < 300; ++i) {
            if (kill(waitPid, 0) != 0 && errno == ESRCH) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if (!std::filesystem::exists(zipPath)) {
        logLine("ERROR: Zip file not found.");
        return 1;
    }

    std::error_code ec;
    std::filesystem::remove_all(extractRoot, ec);
    std::filesystem::create_directories(extractRoot, ec);

    std::string unzipCmd = "unzip -o " + shellQuote(zipPath.string()) + " -d " + shellQuote(extractRoot.string()) + " >/tmp/pholio_apply_update_unzip.log 2>&1";
    logLine("Running unzip...");
    if (std::system(unzipCmd.c_str()) != 0) {
        logLine("ERROR: unzip failed.");
        return 1;
    }

    std::filesystem::path newApp;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(extractRoot, ec)) {
        if (entry.is_directory() && entry.path().extension() == ".app") {
            newApp = entry.path();
            break;
        }
    }

    if (newApp.empty()) {
        logLine("ERROR: No .app found in extracted archive.");
        return 1;
    }

    std::filesystem::remove_all(backupApp, ec);
    if (std::filesystem::exists(targetApp)) {
        std::filesystem::rename(targetApp, backupApp, ec);
        if (ec) {
            logLine("ERROR: Failed to backup existing app: " + ec.message());
            return 1;
        }
    }

    std::filesystem::rename(newApp, targetApp, ec);
    if (ec) {
        logLine("ERROR: Failed to move new app into place: " + ec.message());
        if (std::filesystem::exists(backupApp)) {
            std::error_code restoreEc;
            std::filesystem::rename(backupApp, targetApp, restoreEc);
        }
        return 1;
    }

    std::filesystem::path execPath = targetApp / "Contents/MacOS/Pholio";
    if (!std::filesystem::exists(execPath)) {
        logLine("ERROR: New executable missing after replacement.");
        std::filesystem::remove_all(targetApp, ec);
        if (std::filesystem::exists(backupApp)) {
            std::error_code restoreEc;
            std::filesystem::rename(backupApp, targetApp, restoreEc);
        }
        return 1;
    }

    std::string chmodCmd = "chmod +x " + shellQuote(execPath.string());
    std::string quarantineCmd = "xattr -rd com.apple.quarantine " + shellQuote(targetApp.string()) + " 2>/dev/null";
    std::system(chmodCmd.c_str());
    std::system(quarantineCmd.c_str());

    if (relaunch) {
        logLine("Relaunching app...");
        std::string openCmd = "open -n " + shellQuote(targetApp.string());
        if (std::system(openCmd.c_str()) != 0) {
            std::string fallbackCmd = "nohup " + shellQuote(execPath.string()) + " > /tmp/pholio_relaunch_binary.log 2>&1 < /dev/null &";
            std::system(fallbackCmd.c_str());
        }
    }

    std::filesystem::remove_all(backupApp, ec);
    logLine("apply-update finished successfully");
    return 0;
}

void printHelp() {
    std::cout << "Pholio CLI Helper" << std::endl;
    std::cout << "Usage: Pholio [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help              Show this help message" << std::endl;
    std::cout << "  --version           Show current version" << std::endl;
    std::cout << "  --update            Update to the latest version" << std::endl;
    std::cout << "  --install <version> Install a specific version" << std::endl;
    std::cout << "  --list-versions     List available versions" << std::endl;
    std::cout << "  --apply-update      Internal: apply downloaded update" << std::endl;
}

int main(int argc, char** argv) {
    try {
        // Early startup logging
        const char* home_env = std::getenv("HOME");
        std::filesystem::path startupLogPath = (home_env ? std::filesystem::path(home_env) : std::filesystem::temp_directory_path()) / "pholio_startup.log";
        
        static std::ofstream logFile;
        logFile.open(startupLogPath, std::ios::app);
        
        auto startupLog = [&](const std::string& msg) {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            char timestamp[64];
            std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", std::localtime(&now));
            
            if (logFile.is_open()) {
                logFile << "[" << timestamp << "] " << msg << std::endl;
                logFile.flush();
            }
            std::cout << "[STARTUP] [" << timestamp << "] " << msg << std::endl;
        };

        startupLog("--- Application startup ---");
        startupLog("Path: " + std::string(argv[0]));
        startupLog("Arguments: " + std::to_string(argc));
        for (int i = 0; i < argc; ++i) startupLog("  arg[" + std::to_string(i) + "]: " + argv[i]);

        std::vector<std::string> args(argv, argv + argc);

        if (args.size() > 1) {
            std::string primaryArg = args[1];
            startupLog("Primary argument detected: '" + primaryArg + "'");

            if (primaryArg == "--apply-update") {
                if (logFile.is_open()) logFile.close();
                return runApplyUpdateCli(args);
            }

            if (primaryArg == "--help" || primaryArg == "-h") {
                printHelp();
                return 0;
            } else if (primaryArg == "--version" || primaryArg == "-v") {
                std::cout << core::PROJECT_VERSION << std::endl;
                return 0;
            } else if (primaryArg == "--list-versions") {
                auto versions = core::UpdateManager::getInstance().fetchAvailableVersions();
                for (const auto& v : versions) std::cout << v << std::endl;
                return 0;
            } else if (primaryArg == "--update") {
                auto& um = core::UpdateManager::getInstance();
                startupLog("Checking for updates via CLI...");
                um.checkForUpdates();
                um.waitUntilFinished();
                if (um.getStatus() == core::UpdateStatus::UpdateAvailable) {
                    startupLog("Update found: " + um.getUpdateInfo().latestVersion);
                    um.downloadUpdate();
                    um.waitUntilFinished();
                    if (um.getStatus() == core::UpdateStatus::ReadyToInstall) {
                        startupLog("Installing update and restarting...");
                        if (logFile.is_open()) logFile.close();
                        um.installUpdate(true); 
                    } else {
                        startupLog("ERROR: Download failed.");
                        return 1;
                    }
                } else {
                    startupLog("Already up to date.");
                    return 0;
                }
                return 0;
            } else if (primaryArg == "--install" && args.size() > 2) {
                std::string targetVersion = args[2];
                auto& um = core::UpdateManager::getInstance();
                startupLog("Installing specific version: " + targetVersion);
                if (um.selectVersion(targetVersion)) {
                    um.downloadUpdate();
                    um.waitUntilFinished();
                    if (um.getStatus() == core::UpdateStatus::ReadyToInstall) {
                        um.installUpdate(true);
                    } else {
                        startupLog("ERROR: Download failed.");
                        return 1;
                    }
                } else {
                    startupLog("ERROR: Version not found.");
                    return 1;
                }
                return 0;
            }
            else if (primaryArg.rfind("-", 0) == 0) {
                // macOS system arguments check
                bool isMacSystemArg = false;
                if (primaryArg.rfind("-psn", 0) == 0 || 
                    primaryArg.rfind("-NS", 0) == 0 || 
                    primaryArg.rfind("-Apple", 0) == 0 ||
                    primaryArg.rfind("-X", 0) == 0) {
                    isMacSystemArg = true;
                }

                if (isMacSystemArg) {
                    startupLog("Ignoring macOS system argument: " + primaryArg);
                } else {
#ifdef __APPLE__
                    startupLog("WARNING: Unknown argument detected: " + primaryArg + ". Ignoring because on macOS bundle.");
#else
                    startupLog("ERROR: Unknown argument: " + primaryArg);
                    printHelp();
                    return 1;
#endif
                }
            }
        }

        startupLog("Initializing subsystems...");
        core::CrashHandler::init(core::ConfigManager::getInstance().getCrashesDirectory());
        startupLog("CrashHandler initialized");

        startupLog("Starting main initialization...");
        std::cout << core::PROJECT_NAME << " v" << core::PROJECT_VERSION << " Starting..." << std::endl;

        startupLog("Initializing GLFW...");
        if (!glfwInit()) {
            startupLog("ERROR: glfwInit failed!");
            return 1;
        }

#if defined(__APPLE__)
        startupLog("Setting Mac-specific window hints...");
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#else
        const char* glsl_version = "#version 130";
#endif

        startupLog("Creating GLFW window...");
        GLFWwindow* window = glfwCreateWindow(1280, 720, core::PROJECT_NAME.data(), NULL, NULL);
        if (window == NULL) {
            startupLog("ERROR: glfwCreateWindow failed!");
            return 1;
        }
        
        startupLog("Making context current...");
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        startupLog("Loading configuration...");
        core::ConfigManager::getInstance().load();

        startupLog("Initializing ImGui...");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO();
        static std::string iniPath = core::ConfigManager::getInstance().getImguiConfigPath().string();
        io.IniFilename = iniPath.c_str();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        startupLog("Creating AppWindow...");
        ui::AppWindow appWindow;
        
        auto startTime = std::chrono::steady_clock::now();
        int framesSinceStart = 0;
        int exitCode = 0;

        startupLog("Entering main loop...");

        while (true) {
            glfwPollEvents();

            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
            bool isInitializing = elapsedSeconds < 1.0f; 

            if (glfwWindowShouldClose(window)) {
                if (isInitializing) {
                    glfwSetWindowShouldClose(window, GLFW_FALSE);
                } else {
                    break;
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            appWindow.update();
            appWindow.render();

            if (framesSinceStart < 10) {
                appWindow.clearFlags();
                framesSinceStart++;
            } else {
                if (appWindow.shouldRestart()) {
                    exitCode = appWindow.shouldRebuild() ? 43 : 42;
                    break;
                }
                if (appWindow.shouldClose()) {
                    break;
                }
            }

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.12f, 0.12f, 0.14f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        core::ConfigManager::getInstance().save();
        glfwDestroyWindow(window);
        glfwTerminate();

        startupLog("Application finished with code " + std::to_string(exitCode));
        return exitCode;
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "CRITICAL ERROR: Unknown exception" << std::endl;
        return 1;
    }
}
