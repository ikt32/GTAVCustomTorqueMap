#pragma once
#include <string>

#include "Color.h"

namespace UI {
    void NotifyRaw(const std::string& message, int* prevNotification = nullptr);
    void Notify(int level, const std::string& message);
    void Notify(int level, const std::string& message, bool removePrevious);
}
