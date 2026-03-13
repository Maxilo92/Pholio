#pragma once

#include <imgui.h>
#include <string>
#include "../core/ReportManager.hpp"

namespace ui {

class ReportWindow {
public:
    void render(bool* p_open);

private:
    char m_title[128] = "";
    char m_description[1024] = "";
    char m_email[128] = "";
    int m_typeIdx = 0; // 0: Bug, 1: Feature, 2: Feedback
    int m_priority = 2; // 1: Low, 2: Medium, 3: High
    
    bool m_showSuccess = false;
    bool m_showError = false;
    bool m_showDiagError = false;
    std::string m_diagStatus;
    
    void reset();
};

} // namespace ui
