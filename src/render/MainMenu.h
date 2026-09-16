#pragma once

#include <string>
#include <vector>

namespace render {

struct MenuResult {
    int selected = -1; // index into the entries, -1 if nothing clicked
    bool quit = false;
};

// Draws the centered main-menu window; call between ImGuiLayer begin/endFrame.
MenuResult drawMainMenu(const std::vector<std::string>& entries);

} // namespace render
