#include "render/MainMenu.h"

#include <imgui.h>

namespace render {

MenuResult drawMainMenu(const std::vector<std::string>& entries) {
    MenuResult result;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360.f, 0.f));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin("Space Event Simulator", nullptr, flags)) {
        ImGui::TextDisabled("Choose a simulation");
        ImGui::Separator();
        for (size_t i = 0; i < entries.size(); ++i) {
            if (ImGui::Button(entries[i].c_str(), ImVec2(-1.f, 40.f)))
                result.selected = static_cast<int>(i);
        }
        ImGui::Separator();
        if (ImGui::Button("Quit", ImVec2(-1.f, 0.f)))
            result.quit = true;
    }
    ImGui::End();
    return result;
}

} // namespace render
