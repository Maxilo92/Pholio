#include <iostream>
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <string>
#include <array>
#include <system_error>
#include <fstream>
#include <thread>
#include <signal.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include "core/ConfigManager.hpp"
#include "core/Config.hpp"
#include "core/CrashHandler.hpp"
#include "core/UpdateManager.hpp"
#include "ui/AppWindow.hpp"

static std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

static void appendRestartLog(const std::string& message) {
    const char* home = std::getenv("HOME");
    if (home == nullptr || std::string(home).empty()) return;
    std::error_code ec;
    const auto dir = std::filesystem::path(home) / ".config" / "Pholio";
    std::filesystem::create_directories(dir, ec);
    if (ec) return;
    std::ofstream out(dir / "restart.log", std::ios::app);
    if (!out.is_open()) return;
    out << message << '\n';
}

static bool relaunchDetached(const std::filesystem::path& execPath) {
    if (execPath.empty()) return false;
#if defined(_WIN32)
    std::string cmd = "start \"\" " + shellQuote(execPath.string());
    return std::system(cmd.c_str()) == 0;
#else
    int statusPipe[2] = {-1, -1};
    if (pipe(statusPipe) != 0) return false;
    if (fcntl(statusPipe[1], F_SETFD, FD_CLOEXEC) != 0) {
        close(statusPipe[0]);
        close(statusPipe[1]);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(statusPipe[0]);
        close(statusPipe[1]);
        return false;
    }
    if (pid > 0) {
        close(statusPipe[1]);
        char failureByte = 0;
        const ssize_t bytesRead = read(statusPipe[0], &failureByte, 1);
        close(statusPipe[0]);
        if (bytesRead == 1) {
            const bool ok = failureByte == 'S';
            appendRestartLog(std::string("relaunchDetached explicit ack: ") + (ok ? "success" : "failure"));
            return ok;
        }
#if defined(__APPLE__)
        appendRestartLog("relaunchDetached failed: no explicit ack");
        return false;
#else
        return bytesRead == 0;
#endif
    }

    close(statusPipe[0]);

    if (setsid() < 0) {
        (void)write(statusPipe[1], "E", 1);
        _exit(127);
    }
    const int nullFd = open("/dev/null", O_RDWR);
    if (nullFd >= 0) {
        dup2(nullFd, STDIN_FILENO);
        dup2(nullFd, STDOUT_FILENO);
        dup2(nullFd, STDERR_FILENO);
        if (nullFd > STDERR_FILENO) close(nullFd);
    }
#if defined(__APPLE__)
    std::error_code ackEc;
    const auto ackPath = std::filesystem::temp_directory_path(ackEc) /
                         ("pholio-restart-ack-" + std::to_string(static_cast<long long>(getpid())));
    if (!ackEc) {
        std::filesystem::remove(ackPath, ackEc);
    }

    pid_t launchedPid = fork();
    if (launchedPid < 0) {
        (void)write(statusPipe[1], "E", 1);
        _exit(127);
    }

    if (launchedPid == 0) {
        setenv("PHOLIO_RELAUNCHED", "1", 1);
        if (!ackEc) {
            setenv("PHOLIO_RESTART_ACK_FILE", ackPath.string().c_str(), 1);
        }
        execl(execPath.string().c_str(), execPath.string().c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    bool ackSeen = false;
    for (int i = 0; i < 40; ++i) {
        if (kill(launchedPid, 0) != 0) break;
        if (!ackEc && std::filesystem::exists(ackPath, ackEc) && !ackEc) {
            ackSeen = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (ackSeen && kill(launchedPid, 0) == 0) {
        if (!ackEc) {
            std::filesystem::remove(ackPath, ackEc);
        }
        (void)write(statusPipe[1], "S", 1);
        _exit(0);
    }

    if (!ackEc) {
        std::filesystem::remove(ackPath, ackEc);
    }
    (void)write(statusPipe[1], "E", 1);
    _exit(127);
#else
    setenv("PHOLIO_RELAUNCHED", "1", 1);
    execl(execPath.string().c_str(), execPath.string().c_str(), static_cast<char*>(nullptr));
    (void)write(statusPipe[1], "E", 1);
    _exit(127);
#endif
#endif
}

static std::filesystem::path resolveExecutablePath(const char* argv0) {
#if defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size > 0) {
        std::string buffer(size, '\0');
        if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
            std::error_code ec;
            auto canonical = std::filesystem::weakly_canonical(std::filesystem::path(buffer.c_str()), ec);
            if (!ec) return canonical;
            return std::filesystem::path(buffer.c_str());
        }
    }
#elif defined(_WIN32)
    std::array<char, MAX_PATH> buffer{};
    const DWORD len = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len > 0) {
        return std::filesystem::path(std::string(buffer.data(), len));
    }
#else
    std::array<char, 4096> buffer{};
    const ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (len > 0) {
        buffer[static_cast<size_t>(len)] = '\0';
        return std::filesystem::path(buffer.data());
    }
#endif

    if (argv0 != nullptr && std::string(argv0).size() > 0) {
        std::error_code ec;
        std::filesystem::path path(argv0);
        if (path.is_relative()) {
            path = std::filesystem::absolute(path, ec);
        }
        auto canonical = std::filesystem::weakly_canonical(path, ec);
        if (!ec) return canonical;
        return path;
    }
    return {};
}

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

int main(int argc, char** argv) {
    (void)argc;
    core::CrashHandler::init(core::ConfigManager::getInstance().getCrashesDirectory());

    try {
        std::cout << core::PROJECT_NAME << " v" << core::PROJECT_VERSION << " Starting..." << std::endl;

        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return 1;
        }

#if defined(__APPLE__)
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#else
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

        std::string windowTitle = std::string(core::PROJECT_NAME) + " v" + std::string(core::PROJECT_VERSION);
        GLFWwindow* window = glfwCreateWindow(1280, 720, windowTitle.c_str(), NULL, NULL);
        if (window == NULL) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            return 1;
        }
        
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        glfwShowWindow(window);
        glfwFocusWindow(window);

        core::ConfigManager::getInstance().load();
        if (core::UpdateManager::getInstance().applyPendingUpdateIfRequested()) {
            return 0;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO();
        static std::string iniPath = core::ConfigManager::getInstance().getImguiConfigPath().string();
        io.IniFilename = iniPath.c_str();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        ui::AppWindow appWindow;
        std::cout << "Application initialized. GUI Ready." << std::endl;
        const char* ackFile = std::getenv("PHOLIO_RESTART_ACK_FILE");
        if (ackFile != nullptr && std::string(ackFile).size() > 0) {
            std::ofstream ackOut(ackFile, std::ios::trunc);
            if (ackOut.is_open()) {
                ackOut << "ok";
            }
        }

        const char* delegatedRestartEnv = std::getenv("PHOLIO_RESTART_VIA_EXIT_CODE");
        const bool delegateRestartViaExitCode = delegatedRestartEnv != nullptr && std::string(delegatedRestartEnv) == "1";
        const std::filesystem::path execPath = resolveExecutablePath((argv != nullptr) ? argv[0] : nullptr);
        const char* relaunchedEnv = std::getenv("PHOLIO_RELAUNCHED");
        const bool isRelaunchedInstance = relaunchedEnv != nullptr && std::string(relaunchedEnv) == "1";
        int exitCode = 0;
        auto startTime = std::chrono::steady_clock::now();
        bool forceCloseRequested = false;
        while (true) {
            glfwPollEvents();

            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() - startTime)
                                      .count() / 1000.0f;
            const bool isInitializing = elapsedSeconds < (isRelaunchedInstance ? 30.0f : 1.0f);
            const bool restartAllowed = !isRelaunchedInstance || elapsedSeconds > 10.0f;
            if (glfwWindowShouldClose(window)) {
                if (isInitializing && !forceCloseRequested) {
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

            if (appWindow.shouldRestart()) {
                if (!restartAllowed) {
                    appendRestartLog("restart ignored during relaunched grace window");
                    appWindow.clearFlags();
                    continue;
                }
                const bool rebuildRequested = appWindow.shouldRebuild();
                if (rebuildRequested) {
                    appendRestartLog("restart requested: rebuild path");
                    exitCode = 43;
                    forceCloseRequested = true;
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                } else {
                    if (execPath.empty()) {
                        appendRestartLog("restart requested: normal path rejected (no exec path)");
                        std::cerr << "Restart launch failed: executable path unavailable." << std::endl;
                    } else if (relaunchDetached(execPath)) {
                        appendRestartLog("restart requested: replacement confirmed, closing current");
                        exitCode = 0;
                        forceCloseRequested = true;
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    } else {
                        appendRestartLog("restart requested: replacement failed, keeping current alive");
                        std::cerr << "Restart launch failed. Keeping current instance alive." << std::endl;
                    }
                }
                appWindow.clearFlags();
            }

            if (appWindow.shouldClose()) {
                if (isRelaunchedInstance && elapsedSeconds < 15.0f) {
                    appendRestartLog("close ignored during relaunched grace window");
                    appWindow.clearFlags();
                } else {
                    forceCloseRequested = true;
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
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

        if (exitCode == 43) {
            if (delegateRestartViaExitCode) {
                return exitCode;
            }

            if (execPath.empty()) {
                std::cerr << "Failed to resolve executable path for relaunch." << std::endl;
                return 1;
            }
            std::cerr << "Rebuild+restart requested without wrapper. Relaunching current executable without rebuild." << std::endl;
            if (!relaunchDetached(execPath)) {
                std::cerr << "Failed to relaunch executable: " << execPath.string() << std::endl;
                return 1;
            }
            return 0;
        }

        std::cout << core::PROJECT_NAME << " Finished" << std::endl;
        return exitCode;
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN CRITICAL ERROR" << std::endl;
        return 1;
    }
}
