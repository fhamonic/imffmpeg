#ifndef IMFFMPEG_BUBBLESORT_GUI_HPP
#define IMFFMPEG_BUBBLESORT_GUI_HPP

#include <algorithm>
#include <optional>
#include <random>
#include <ranges>
#include <stack>
#include <vector>

#include "command_gui.hpp"

namespace ImFFmpeg {

class TranscodeGUI : public CommandGUI {
private:
    int length = 100;

public:
    void showControlPanel(ImVec2 pos, ImVec2 size) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Quicksort Control Panel", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Nb Values:");
        ImGui::SameLine();
        ImGui::InputInt("", &length, 0);

        ImGui::End();
    }
    void showValues(ImVec2 pos, ImVec2 size) const {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Values", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::End();
    }
    void show(ImVec2 pos, ImVec2 size) {
        showControlPanel(ImVec2(pos.x, pos.y + size.y - 100),
                         ImVec2(size.x, 100));
    };
    const char * name() const { return "Transcode"; }
};

}  // namespace ImFFmpeg

#endif  // ImFFmpeg_BUBBLESORT_GUI_HPP