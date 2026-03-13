#pragma once

#include <string>
#include <vector>

namespace ui {

class DebugWindow {
public:
    void render(bool* p_open);

private:
    void renderApiLogs();
    void renderSystemInfo();
    void renderPerformance();
};

} // namespace ui
