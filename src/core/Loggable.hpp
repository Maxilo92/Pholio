#pragma once

#include "../ui/LogWindow.hpp"
#include <string>

namespace core {

class Loggable {
public:
    explicit Loggable(ui::LogWindow& logWindow) : m_logWindow(logWindow) {}
    virtual ~Loggable() = default;

protected:
    void info(const std::string& message) const { m_logWindow.info(message); }
    void warn(const std::string& message) const { m_logWindow.warn(message); }
    void error(const std::string& message) const { m_logWindow.error(message); }
    void success(const std::string& message) const { m_logWindow.success(message); }

    ui::LogWindow& m_logWindow;
};

} // namespace core
