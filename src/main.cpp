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

static bool relaunchDetached(const std::string& execPath) {
    if (execPath.empty()) return false;
    std::string cmd = "nohup " + shellQuote(execPath) + " > /tmp/pholio_relaunch.log 2>&1 < /dev/null &";
    return std::system(cmd.c_str()) == 0;
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
        int framesSinceStart = 0;
        while (true) {
            glfwPollEvents();

            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() - startTime)
                                      .count() / 1000.0f;
            const bool isInitializing = elapsedSeconds < 1.0f;
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
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }

                if (appWindow.shouldClose()) {
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

        if (exitCode == 42) {
            const char* delegatedRestartEnv = std::getenv("PHOLIO_RESTART_VIA_EXIT_CODE");
            const bool delegateRestartViaExitCode = delegatedRestartEnv != nullptr && std::string(delegatedRestartEnv) == "1";
            if (delegateRestartViaExitCode) {
                return 42;
            }

            std::filesystem::path execPath;
            if (argv != nullptr && argv[0] != nullptr && std::string(argv[0]).size() > 0) {
                execPath = std::filesystem::absolute(argv[0]);
            }
            if (!relaunchDetached(execPath.string())) {
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
