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

#if defined(__APPLE__)
static std::filesystem::path findAppBundlePath(const std::filesystem::path& execPath) {
    if (execPath.empty()) return {};
    auto p = execPath;
    for (int i = 0; i < 4 && !p.empty(); ++i) {
        if (p.extension() == ".app") return p;
        p = p.parent_path();
    }
    return {};
}
#endif

static bool relaunchDetached(const std::filesystem::path& execPath) {
    if (execPath.empty()) return false;
#if defined(_WIN32)
    std::string cmd = "start \"\" " + shellQuote(execPath.string());
    return std::system(cmd.c_str()) == 0;
#else
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid > 0) return true;

    if (setsid() < 0) _exit(127);
    const int nullFd = open("/dev/null", O_RDWR);
    if (nullFd >= 0) {
        dup2(nullFd, STDIN_FILENO);
        dup2(nullFd, STDOUT_FILENO);
        dup2(nullFd, STDERR_FILENO);
        if (nullFd > STDERR_FILENO) close(nullFd);
    }
#if defined(__APPLE__)
    const auto appBundlePath = findAppBundlePath(execPath);
    if (!appBundlePath.empty()) {
        execlp("open", "open", "-n", appBundlePath.string().c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
#endif

    execl(execPath.string().c_str(), execPath.string().c_str(), static_cast<char*>(nullptr));
    _exit(127);
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

        int exitCode = 0;
        auto startTime = std::chrono::steady_clock::now();
        bool forceCloseRequested = false;
        while (true) {
            glfwPollEvents();

            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() - startTime)
                                      .count() / 1000.0f;
            const bool isInitializing = elapsedSeconds < 1.0f;
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
                exitCode = appWindow.shouldRebuild() ? 43 : 42;
                forceCloseRequested = true;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            if (appWindow.shouldClose()) {
                forceCloseRequested = true;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
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

        if (exitCode == 42 || exitCode == 43) {
            const bool rebuildRequested = exitCode == 43;
            const char* delegatedRestartEnv = std::getenv("PHOLIO_RESTART_VIA_EXIT_CODE");
            const bool delegateRestartViaExitCode = delegatedRestartEnv != nullptr && std::string(delegatedRestartEnv) == "1";
            if (delegateRestartViaExitCode) {
                return exitCode;
            }

            std::filesystem::path execPath = resolveExecutablePath((argv != nullptr) ? argv[0] : nullptr);
            if (execPath.empty()) {
                std::cerr << "Failed to resolve executable path for relaunch." << std::endl;
                return 1;
            }
            if (rebuildRequested) {
                std::cerr << "Rebuild+restart requested without wrapper. Relaunching current executable without rebuild." << std::endl;
            }
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
