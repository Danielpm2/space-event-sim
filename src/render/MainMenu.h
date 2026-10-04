#pragma once

#include <string>
#include <vector>

namespace render {

struct MenuEntry {
    std::string name;
    std::string description;
};

struct MenuResult {
    int selected = -1; // index into the entries, -1 if nothing clicked
    bool quit = false;
};

// Draws the centered main-menu window; call between ImGuiLayer begin/endFrame.
// A non-empty `error` is shown under the entries (e.g. a shader failure).
MenuResult drawMainMenu(const std::vector<MenuEntry>& entries, const std::string& error);

} // namespace render
