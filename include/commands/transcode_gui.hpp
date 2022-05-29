#ifndef IMFFMPEG_TRANSCODE_GUI_HPP
#define IMFFMPEG_TRANSCODE_GUI_HPP

#include <filesystem>
#include <iostream>
#include <vector>

#include "command_gui.hpp"
#include "utils/ffprobe_helper.hpp"

#include "ImGuiFileDialog/ImGuiFileDialog.h"

#include "boost/process.hpp"
namespace bp = boost::process;

#include "nlohmann/json.hpp"

namespace ImFFmpeg {

class TranscodeGUI : public CommandGUI {
private:
    ImGuiFileDialog source_dir_dialog;
    char source_dir_path_buffer[1024] = "";
    ImGuiFileDialog output_dir_dialog;
    char output_dir_path_buffer[1024] = "";
    char extensions_buffer[1024] = ".ts,.mp4,.mkv,.avi";
    std::vector<std::filesystem::path> files;
    std::vector<std::filesystem::path>::const_iterator file_iterator;
    bool transcoding = false;
    bp::child child_process;
    bp::ipstream progress_stream;

    std::vector<std::string> split(std::string s, std::string delimiter) const {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }

    void scan() {
        if(transcoding) return;
        std::vector<std::string> extensions = split(extensions_buffer, ",");
        files.clear();
        for(auto && p :
            std::filesystem::directory_iterator(source_dir_path_buffer)) {
            if(std::find(extensions.cbegin(), extensions.cend(),
                         p.path().extension()) != extensions.cend()) {
                files.push_back(p);
            }
        }
    }

    std::string make_command() const {
        std::stringstream ss;
        ss << "ffmpeg -i \"" << file_iterator->string() << '\"';

        nlohmann::json file_properties = ffprobe(*file_iterator);
        int stream_cpt = 0;
        for(auto && stream : file_properties["streams"]) {
            int stream_index = stream["index"].get<int>();
            std::string stream_type = stream["codec_type"].get<std::string>();

            if(stream_type == "video") {
                ss << " -map 0:" << stream_index << " -c:" << stream_cpt;
                if(stream["codec_name"].get<std::string>() == "h264")
                    ss << " copy";
                else {
                    ss << " libx264 -preset slow -crf 18";
                }
            } else if(stream_type == "audio") {
                ss << " -map 0:" << stream_index << " -c:" << stream_cpt;
                if(stream["codec_name"].get<std::string>() == "aac")
                    ss << " copy";
                else {
                    ss << " aac";
                }
                if(stream["tags"].contains("language")) {
                    if(stream["tags"]["language"].get<std::string>() == "eng")
                        ss << " -disposition:" << stream_cpt << " default";
                }
            } else if(stream_type == "subtitle") {
                ss << " -map 0:" << stream_index << " -c:" << stream_cpt
                   << " copy";
                if(stream["tags"].contains("language")) {
                    if(stream["tags"]["language"].get<std::string>() == "fre")
                        ss << " -disposition:" << stream_cpt << " default";
                }
            }
            ++stream_cpt;
        }

        std::filesystem::path output_file_path =
            std::filesystem::path(output_dir_path_buffer) /
            file_iterator->stem();

        ss << " -progress url \"" << output_file_path.string() << ".mkv\"";

        return ss.str();
    }

    void transcode() {
        if(!transcoding) return;
        if(child_process.running()) {
            return;
        }
        if(file_iterator == files.cend()) {
            transcoding = false;
            return;
        }

        std::string cmd = make_command();
        progress_stream = bp::ipstream();
        child_process = bp::child(cmd, bp::std_out > progress_stream);

        ++file_iterator;
    }

public:
    void showControlPanel(ImVec2 pos, ImVec2 size) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Probe Control Panel", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::InputText("##source_dir", source_dir_path_buffer, 1024);
        ImGui::SameLine();
        if(ImGui::Button("Choose source dir")) {
            source_dir_dialog.OpenDialog("ChooseDirDlgKey",
                                         "Choose a Directory", nullptr, ".");
        }
        if(source_dir_dialog.Display("ChooseDirDlgKey")) {
            if(source_dir_dialog.IsOk()) {
                std::string tmp_dir_path = source_dir_dialog.GetFilePathName();
                std::strcpy(source_dir_path_buffer, tmp_dir_path.c_str());
            }
            source_dir_dialog.Close();
        }

        ImGui::PushItemWidth(0);
        ImGui::InputText("extensions to seek", extensions_buffer, 1024);
        ImGui::SameLine();
        if(ImGui::Button("Scan Files")) {
            scan();
        }
        ImGui::SameLine();
        ImGui::Text((std::to_string(files.size()) + " Files found").c_str());

        ImGui::InputText("##output_dir", output_dir_path_buffer, 1024);
        ImGui::SameLine();
        if(ImGui::Button("Choose output dir")) {
            output_dir_dialog.OpenDialog("ChooseDirDlgKey",
                                         "Choose a Directory", nullptr, ".");
        }
        if(output_dir_dialog.Display("ChooseDirDlgKey")) {
            if(output_dir_dialog.IsOk()) {
                std::string tmp_dir_path = output_dir_dialog.GetFilePathName();
                std::strcpy(output_dir_path_buffer, tmp_dir_path.c_str());
            }
            output_dir_dialog.Close();
        }

        if(ImGui::Button("Transcode")) {
            file_iterator = files.cbegin();
            transcoding = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Stop")) {
            child_process.terminate();
            transcoding = false;
        }
        if(transcoding) {
            ImGui::SameLine();
            ImGui::Text("Transcoding: %d / %d",
                        std::distance(files.cbegin(), file_iterator),
                        files.size());
        }

        ImGui::End();
    }
    void showProgress(ImVec2 pos, ImVec2 size) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Progress", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::End();
    }
    void show(ImVec2 pos, ImVec2 size) {
        showControlPanel(pos, ImVec2(size.x, 125));
        showProgress(ImVec2(pos.x, pos.y + 125), ImVec2(size.x, size.y - 125));
        transcode();
    };
    const char * name() const { return "Transcode"; }
};

}  // namespace ImFFmpeg

#endif  // IMFFMPEG_TRANSCODE_GUI_HPP