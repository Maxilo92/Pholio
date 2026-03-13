#pragma once

#include "../engine/Worker.hpp"
#include <deque>
#include <vector>

namespace ui {

class ProgressWindow {
public:
    explicit ProgressWindow(engine::Worker& worker);

    void render();

private:
    engine::Worker& m_worker;
    
    // Performance history for ImPlot
    std::deque<float> m_fpsHistory;
    std::deque<float> m_mbpsHistory;
    float m_timeElapsed = 0.0f;
    bool m_workerWasRunning = false;
    
    void updateHistory(float dt);
    static std::string formatSize(uint64_t bytes);
};

} // namespace ui
