#include "UIUtils.h"

#include "inc/natives.h"
#include "inc/enums.h"
#include "fmt/format.h"
#include <algorithm>

#include "../Constants.h"

namespace {
    int notificationHandle = 0;
}

void UI::Notify(int level, const std::string& message) {
    Notify(level, message, true);
}

void UI::Notify(int, const std::string& message, bool removePrevious) {
    int* notifHandleAddr = nullptr;
    if (removePrevious) {
        notifHandleAddr = &notificationHandle;
    }
    NotifyRaw(fmt::format("{}\n{}", Constants::NotificationPrefix, message), notifHandleAddr);
}

void UI::NotifyRaw(const std::string &message, int *prevNotification) {
    if (prevNotification != nullptr && *prevNotification != 0) {
        HUD::THEFEED_REMOVE_ITEM(*prevNotification);
    }
    HUD::BEGIN_TEXT_COMMAND_THEFEED_POST("STRING");

    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(message.c_str());

    int id = HUD::END_TEXT_COMMAND_THEFEED_POST_TICKER(false, false);
    if (prevNotification != nullptr) {
        *prevNotification = id;
    }
}
