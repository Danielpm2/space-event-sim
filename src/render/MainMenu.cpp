#include "render/MainMenu.h"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>

namespace render {

MenuResult drawMainMenu(const std::vector<MenuEntry>& entries, const std::string& error) {
    MenuResult result;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(660.f, vp->Size.x - 40.f), 0.f));
    ImGui::SetNextWindowBgAlpha(0.72f);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin("##mainmenu", nullptr, flags)) {
        const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        const float base = ImGui::GetFontSize();

        const char* title = "SPACE EVENT SIMULATOR";
        const float titleSize = base * 1.5f;
        const ImVec2 ts = font->CalcTextSizeA(titleSize, FLT_MAX, 0.f, title);
        const ImVec2 cur = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        dl->AddText(font, titleSize, ImVec2(cur.x + (width - ts.x) * 0.5f, cur.y), ImGui::GetColorU32(accent), title);
        ImGui::Dummy(ImVec2(0.f, ts.y + 2.f));

        const char* sub = "Choose a simulation";
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(sub).x) * 0.5f);
        ImGui::TextDisabled("%s", sub);
        ImGui::Spacing();

        const float cardH = base * 3.6f;
        for (size_t i = 0; i < entries.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            const ImVec2 p = ImGui::GetCursorScreenPos();
            if (ImGui::Button("##card", ImVec2(-1.f, cardH)))
                result.selected = static_cast<int>(i);
            const ImVec2 size = ImGui::GetItemRectSize();
            if (i < 9 && ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + static_cast<int>(i)), false))
                result.selected = static_cast<int>(i);

            const float pad = 14.f;
            dl->AddText(font, base * 1.2f, ImVec2(p.x + pad, p.y + base * 0.55f), IM_COL32(240, 245, 255, 255),
                        entries[i].name.c_str());
            dl->AddText(font, base * 0.9f, ImVec2(p.x + pad, p.y + base * 2.05f), IM_COL32(170, 185, 210, 255),
                        entries[i].description.c_str());
            if (i < 9) {
                char key[4];
                std::snprintf(key, sizeof(key), "%zu", i + 1);
                const ImVec2 ks = font->CalcTextSizeA(base * 1.1f, FLT_MAX, 0.f, key);
                dl->AddText(font, base * 1.1f, ImVec2(p.x + size.x - ks.x - pad, p.y + base * 0.55f),
                            ImGui::GetColorU32(accent), key);
            }
            ImGui::PopID();
        }

        if (!error.empty()) {
            ImGui::Spacing();
            ImGui::PushTextWrapPos(0.f);
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.45f, 1.f), "%s", error.c_str());
            ImGui::PopTextWrapPos();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("Quit", ImVec2(110.f, 0.f)))
            result.quit = true;
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Click a card or press 1-%d   |   Esc quits",
                            static_cast<int>(std::min<size_t>(entries.size(), 9)));
    }
    ImGui::End();
    return result;
}

} // namespace render
