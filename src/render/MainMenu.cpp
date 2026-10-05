#include "render/MainMenu.h"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>

namespace render {

namespace {

ImFont* fontAt(int index) {
    ImVector<ImFont*>& fonts = ImGui::GetIO().Fonts->Fonts;
    return index < fonts.Size ? fonts[index] : ImGui::GetFont();
}

float textWidth(ImFont* font, const char* text) { return font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.f, text).x; }

} // namespace

MenuResult drawMainMenu(const std::vector<MenuEntry>& entries, const std::string& error) {
    MenuResult result;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(680.f, vp->Size.x - 40.f), 0.f));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.f, 26.f));
    ImGui::SetNextWindowBgAlpha(0.82f);
    if (ImGui::Begin("##mainmenu", nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImFont* body = ImGui::GetFont();
        ImFont* cardTitle = fontAt(1);
        ImFont* heading = fontAt(2);
        const float base = ImGui::GetFontSize();
        const float width = ImGui::GetContentRegionAvail().x;

        const char* title = "Space Event Simulator";
        const ImVec2 cur = ImGui::GetCursorScreenPos();
        dl->AddText(heading, heading->FontSize, ImVec2(cur.x + (width - textWidth(heading, title)) * 0.5f, cur.y),
                    IM_COL32(240, 240, 242, 255), title);
        ImGui::Dummy(ImVec2(0.f, heading->FontSize + base * 1.6f));

        const float rowH = base * 3.6f;
        for (size_t i = 0; i < entries.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const bool clicked = ImGui::InvisibleButton("##row", ImVec2(width, rowH));
            const bool hovered = ImGui::IsItemHovered();
            if (hovered)
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (clicked || (i < 9 && ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + static_cast<int>(i)), false)))
                result.selected = static_cast<int>(i);

            const ImVec2 q(p.x + width, p.y + rowH);
            if (hovered)
                dl->AddRectFilled(p, q, IM_COL32(255, 255, 255, ImGui::IsItemActive() ? 28 : 16), 4.f);

            const float pad = base * 1.1f;
            dl->AddText(cardTitle, cardTitle->FontSize, ImVec2(p.x + pad, p.y + rowH * 0.5f - cardTitle->FontSize * 1.05f),
                        IM_COL32(236, 236, 240, 255), entries[i].name.c_str());
            dl->AddText(body, base * 0.95f, ImVec2(p.x + pad, p.y + rowH * 0.5f + base * 0.15f),
                        IM_COL32(160, 162, 170, 255), entries[i].description.c_str());

            if (i < 9) {
                char key[4];
                std::snprintf(key, sizeof(key), "%zu", i + 1);
                const ImVec2 ts = body->CalcTextSizeA(base, FLT_MAX, 0.f, key);
                dl->AddText(body, base, ImVec2(q.x - pad - ts.x, p.y + (rowH - ts.y) * 0.5f),
                            IM_COL32(110, 112, 120, 255), key);
            }
            if (i + 1 < entries.size())
                dl->AddLine(ImVec2(p.x + pad, q.y), ImVec2(q.x - pad, q.y), IM_COL32(255, 255, 255, 18));
            ImGui::PopID();
        }

        if (!error.empty()) {
            ImGui::Spacing();
            ImGui::PushTextWrapPos(0.f);
            ImGui::TextColored(ImVec4(1.f, 0.45f, 0.45f, 1.f), "%s", error.c_str());
            ImGui::PopTextWrapPos();
        }

        ImGui::Dummy(ImVec2(0.f, base * 0.3f));
        ImGui::Indent(base * 1.1f);
        if (ImGui::Button("Quit", ImVec2(110.f, 0.f)))
            result.quit = true;
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Click a card or press 1-%d   |   Esc quits",
                            static_cast<int>(std::min<size_t>(entries.size(), 9)));
    }
    ImGui::End();
    ImGui::PopStyleVar();
    return result;
}

} // namespace render
