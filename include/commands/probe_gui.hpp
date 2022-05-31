#ifndef IMFFMPEG_PROBE_GUI_HPP
#define IMFFMPEG_PROBE_GUI_HPP

#include <cstring>
#include <filesystem>
#include <string>

#include "command_gui.hpp"
#include "utils/ffprobe_helper.hpp"

#include "ImGuiFileDialog/ImGuiFileDialog.h"

#include "boost/process.hpp"
namespace bp = boost::process;

#include "nlohmann/json.hpp"

namespace ImFFmpeg {

class ProbeGUI : public CommandGUI {
private:
    char file_path[1024];
    nlohmann::json file_properties;

public:
    void showControlPanel(ImVec2 pos, ImVec2 size) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Probe Control Panel", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("File to probe:");
        ImGui::SameLine();
        ImGui::InputText("", file_path, 1024);
        ImGui::SameLine();
        if(ImGui::Button("...")) {
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseFileDlgKey", "Choose File", ".mkv,.mp4,.avi", file_path);
        }
        if(ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if(ImGuiFileDialog::Instance()->IsOk()) {
                std::string tmp_file_path =
                    ImGuiFileDialog::Instance()->GetFilePathName();
                std::strcpy(file_path, tmp_file_path.c_str());
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if(ImGui::Button("Probe")) {
            file_properties = ffprobe(std::filesystem::path(file_path));
        }

        ImGui::End();
    }
    void showInfos(ImVec2 pos, ImVec2 size) const {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Streams", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        if(file_properties.contains("streams")) {
            for(auto && stream : file_properties["streams"]) {
                if(ImGui::CollapsingHeader(
                       ('#' + std::to_string(stream["index"].get<int>()) + ' ' +
                        stream["codec_type"].get<std::string>())
                           .c_str())) {
                    if(stream["tags"].contains("title"))
                        ImGui::Text(("title: " +
                                     stream["tags"]["title"].get<std::string>())
                                        .c_str());

                    ImGui::Text(("codec_name: " +
                                 stream["codec_name"].get<std::string>())
                                    .c_str());

                    if(stream["tags"].contains("language"))
                        ImGui::Text(
                            ("language: " +
                             stream["tags"]["language"].get<std::string>())
                                .c_str());
                }
            }
        }

        ImGui::End();
    }
    void show(ImVec2 pos, ImVec2 size) {
        showControlPanel(pos, ImVec2(size.x, 100));

        showInfos(ImVec2(pos.x, pos.y + 100), ImVec2(size.x, size.y - 100));
    };
    const char * name() const { return "Probe"; }
};

}  // namespace ImFFmpeg

#endif  // IMFFMPEG_PROBE_GUI_HPP