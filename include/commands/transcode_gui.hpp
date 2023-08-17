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

    bool contains(const std::vector<std::string> & strs,
                  const std::string & str) const {
        return std::find(strs.cbegin(), strs.cend(), str) != strs.cend();
    }

    void scan() {
        if(transcoding) return;
        std::vector<std::string> extensions = split(extensions_buffer, ",");
        files.clear();
        for(auto && p :
            std::filesystem::directory_iterator(source_dir_path_buffer)) {
            if(contains(extensions, p.path().extension())) {
                files.push_back(p);
            }
        }
    }

    std::string make_command() const {
        std::stringstream ss;
        ss << "ffmpeg -i \"" << file_iterator->string() << '\"';

        nlohmann::json file_properties = ffprobe(*file_iterator);

        std::vector<std::string> accepted_video_codecs =
            split(accepted_video_codecs_buffer, ",");
        std::string fallback_video_codec = fallback_video_codec_buffer;
        std::string preset = presets[preset_index];
        std::vector<std::string> accepted_audio_codecs =
            split(accepted_audio_codecs_buffer, ",");
        // std::vector<std::string> accepted_audio_langs =
        //     split(accepted_audio_langs_buffer, ",");
        std::string fallback_audio_codec = fallback_audio_codec_buffer;

        int stream_cpt = 0;
        for(auto && stream : file_properties["streams"]) {
            std::string stream_type = stream["codec_type"].get<std::string>();
            if(stream_type != "video") continue;
            int stream_index = stream["index"].get<int>();
            ss << " -map 0:" << stream_index << " -c:" << stream_cpt;
            if(contains(accepted_video_codecs,
                        stream["codec_name"].get<std::string>())) {
                ss << " copy";
            } else {
                ss << ' ' << fallback_video_codec << " -preset " << preset
                   << " -crf " << crf;
                if(custom_video_profile) {
                    ss << " -profile:" << stream_cpt << ' ' << video_profile;
                }
                if(custom_pixel_format) {
                    ss << " -pix_fmt:" << stream_cpt << ' ' << pixel_format;
                }
            }
            if(custom_target_screen) {
                int stream_width = stream["width"].get<int>();
                int stream_height = stream["height"].get<int>();
                int screen_width = target_screens_widths[target_screen_index];
                int screen_height = target_screens_heights[target_screen_index];

                if(stream_width > screen_width ||
                   stream_height > screen_height) {
                    float stream_ratio = stream_width / stream_height;
                    float screen_ratio = screen_width / screen_height;
                    ss << " -filter:" << stream_cpt << " scale="
                       << (stream_ratio < screen_ratio ? -1 : screen_width)
                       << ':'
                       << (stream_ratio > screen_ratio ? -1 : screen_height);
                }
            }
            ++stream_cpt;
        }
        for(auto && stream : file_properties["streams"]) {
            std::string stream_type = stream["codec_type"].get<std::string>();
            if(stream_type != "audio") continue;
            int stream_index = stream["index"].get<int>();

            ss << " -map 0:" << stream_index << " -c:" << stream_cpt;
            if(contains(accepted_audio_codecs,
                        stream["codec_name"].get<std::string>())) {
                ss << " copy";
            } else {
                ss << ' ' << fallback_audio_codec;
            }
            if(custom_default_audio_lang &&
               stream["tags"].contains("language")) {
                if(stream["tags"]["language"].get<std::string>() ==
                   default_audio_lang) {
                    ss << " -disposition:" << stream_cpt << " default";
                } else {
                    ss << " -disposition:" << stream_cpt << " -default";
                }
            }
            ++stream_cpt;
        }
        for(auto && stream : file_properties["streams"]) {
            std::string stream_type = stream["codec_type"].get<std::string>();
            if(stream_type != "subtitle") continue;
            int stream_index = stream["index"].get<int>();
            ss << " -map 0:" << stream_index << " -c:" << stream_cpt << " copy";
            if(stream["tags"].contains("language")) {
                if(stream["tags"]["language"].get<std::string>() == "fre") {
                    ss << " -disposition:" << stream_cpt << " default";
                } else {
                    ss << " -disposition:" << stream_cpt << " -default";
                }
            }
            ++stream_cpt;
        }
        if(fast_start) {
            ss << " -movflags +faststart";
        }

        // for(auto && stream : file_properties["streams"]) {
        //     int stream_index = stream["index"].get<int>();
        //     std::string stream_type =
        //     stream["codec_type"].get<std::string>();

        //     if(stream_type == "video") {
        //         ss << " -map 0:" << stream_index << " -c:" << stream_cpt;
        //         if(contains(accepted_video_codecs,
        //                     stream["codec_name"].get<std::string>())) {
        //             ss << " copy";
        //         } else {
        //             ss << ' ' << fallback_video_codec << " -preset " <<
        //             preset
        //                << " -crf " << crf;
        //         }
        //         if(custom_target_screen) {
        //             int stream_width = stream["width"].get<int>();
        //             int stream_height = stream["height"].get<int>();
        //             int screen_width =
        //                 target_screens_widths[target_screen_index];
        //             int screen_height =
        //                 target_screens_heights[target_screen_index];

        //             if(stream_width > screen_width ||
        //                stream_height > screen_height) {
        //                 float stream_ratio = stream_width / stream_height;
        //                 float screen_ratio = screen_width / screen_height;
        //                 ss << " -filter:" << stream_cpt << " scale="
        //                    << (stream_ratio < screen_ratio ? -1 :
        //                    screen_width)
        //                    << ':'
        //                    << (stream_ratio > screen_ratio ? -1
        //                                                    : screen_height);
        //             }
        //             if(custom_video_profile) {
        //                 ss << " -profile:" << stream_cpt << ' '
        //                    << video_profile;
        //             }
        //         }
        //     } else if(stream_type == "audio") {
        //         ss << " -map 0:" << stream_index << " -c:" << stream_cpt;
        //         if(contains(accepted_audio_codecs,
        //                     stream["codec_name"].get<std::string>())) {
        //             ss << " copy";
        //         } else {
        //             ss << ' ' << fallback_audio_codec;
        //         }
        //         if(stream["tags"].contains("language")) {
        //             if(stream["tags"]["language"].get<std::string>() ==
        //             "eng") {
        //                 ss << " -disposition:" << stream_cpt << " default";
        //             } else {
        //                 ss << " -disposition:" << stream_cpt << " -default";
        //             }
        //         }
        //     } else if(stream_type == "subtitle") {
        //         ss << " -map 0:" << stream_index << " -c:" << stream_cpt
        //            << " copy";
        //         if(stream["tags"].contains("language")) {
        //             if(stream["tags"]["language"].get<std::string>() ==
        //             "fre") {
        //                 ss << " -disposition:" << stream_cpt << " default";
        //             } else {
        //                 ss << " -disposition:" << stream_cpt << " -default";
        //             }
        //         }
        //     }
        //     ++stream_cpt;
        // }

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
        // std::cout << cmd << std::endl;
        progress_stream = bp::ipstream();
        child_process = bp::child(cmd, bp::std_out > progress_stream);

        ++file_iterator;
    }

public:
    ImGuiFileDialog source_dir_dialog;
    char source_dir_path_buffer[1024] = "";
    char extensions_buffer[64] = ".ts,.mp4,.mkv,.avi";
    std::vector<std::filesystem::path> files;
    std::vector<std::filesystem::path>::const_iterator file_iterator;
    void showSourceDirHeader() {
        if(ImGui::CollapsingHeader("Input")) {
            ImGui::Text("Input directory:   ");
            ImGui::SameLine();
            ImGui::InputText("##source_dir", source_dir_path_buffer, 1024);
            ImGui::SameLine();
            if(ImGui::Button("...##source_dir_btn")) {
                source_dir_dialog.OpenDialog("ChooseDirDlgKey",
                                             "Choose a Directory", nullptr,
                                             source_dir_path_buffer);
            }
            if(source_dir_dialog.Display("ChooseDirDlgKey")) {
                if(source_dir_dialog.IsOk()) {
                    std::string tmp_dir_path =
                        source_dir_dialog.GetFilePathName();
                    std::strcpy(source_dir_path_buffer, tmp_dir_path.c_str());
                }
                source_dir_dialog.Close();
            }

            ImGui::Text("Extensions:        ");
            ImGui::SameLine();
            ImGui::InputText("##source_extensions", extensions_buffer, 64);
        }
    }

    char accepted_video_codecs_buffer[64] = "h264,vp9";
    char fallback_video_codec_buffer[16] = "libx264";
    int crf = 18;
    std::array<const char *, 9> presets = {
        "ultrafast", "superfast", "veryfast", "faster",  "fast",
        "medium",    "slow",      "slower",   "veryslow"};
    int preset_index = 6;
    bool custom_video_profile = true;
    char video_profile[16] = "high";
    bool custom_pixel_format = true;
    char pixel_format[16] = "yuv420p";
    bool custom_target_screen = true;
    std::array<const char *, 5> target_screens = {
        "800x240 3DS", "720×480 16:9", "1280x720 16:9", "1920x1080 16:9", "3840x2060 16:9"};
    std::array<int, 5> target_screens_widths = {800, 720, 1280, 1920, 3840};
    std::array<int, 5> target_screens_heights = {240, 720, 1080, 2060};
    int target_screen_index = 2;
    bool fast_start;
    void showVideoHeader() {
        if(ImGui::CollapsingHeader("Video")) {
            ImGui::Text("Accepted codecs:   ");
            ImGui::SameLine();
            ImGui::InputText("##accepted_video_codecs",
                             accepted_video_codecs_buffer, 64);

            ImGui::Text("Fallback codec:    ");
            ImGui::SameLine();
            ImGui::InputText("##fallback_video_codec",
                             fallback_video_codec_buffer, 16);

            ImGui::Text("CRF:               ");
            ImGui::SameLine();
            ImGui::SliderInt("##video_crf", &crf, 0, 50);

            ImGui::Text("Preset:            ");
            ImGui::SameLine();
            ImGui::Combo("##preset", &preset_index, presets.data(),
                         presets.size());

            ImGui::Text("Profile:       ");
            ImGui::SameLine();
            ImGui::Checkbox("##custom_video_profile", &custom_video_profile);
            ImGui::SameLine();
            ImGui::InputText("##video_profile", video_profile, 16);

            ImGui::Text("Pixel format:  ");
            ImGui::SameLine();
            ImGui::Checkbox("##custom_pixel_format", &custom_pixel_format);
            ImGui::SameLine();
            ImGui::InputText("##pixel_format", pixel_format, 16);

            ImGui::Text("Target screen: ");
            ImGui::SameLine();
            ImGui::Checkbox("##custom_target_screen", &custom_target_screen);
            ImGui::SameLine();
            ImGui::Combo("##target_screen", &target_screen_index,
                         target_screens.data(), target_screens.size());
            
            ImGui::Text("Move flags for fast start:  ");
            ImGui::SameLine();
            ImGui::Checkbox("##fast_start", &fast_start);
        }
    }

    char accepted_audio_codecs_buffer[64] = "flac,aac,eac3,opus";
    char fallback_audio_codec_buffer[16] = "aac";
    bool custom_default_audio_lang = false;
    char default_audio_lang[8] = "eng";
    bool custom_audio_bitrate = false;
    int audio_bitrate = 128;
    void showAudioHeader() {
        if(ImGui::CollapsingHeader("Audio")) {
            ImGui::Text("Accepted codecs:   ");
            ImGui::SameLine();
            ImGui::InputText("##accepted_audio_codecs",
                             accepted_audio_codecs_buffer, 64);

            ImGui::Text("Fallback codec:    ");
            ImGui::SameLine();
            ImGui::InputText("##fallback_audio_codec",
                             fallback_audio_codec_buffer, 16);

            ImGui::Text("Default lang.: ");
            ImGui::SameLine();
            ImGui::Checkbox("##custom_default_audio_lang",
                            &custom_default_audio_lang);
            ImGui::SameLine();
            ImGui::InputText("##default_audio_lang", default_audio_lang, 8);

            ImGui::Text("Bitrate:       ");
            ImGui::SameLine();
            ImGui::Checkbox("##custom_audio_bitrate", &custom_audio_bitrate);
            ImGui::SameLine();
            ImGui::InputInt("##audio_bitrate", &audio_bitrate);
        }
    }

    ImGuiFileDialog output_dir_dialog;
    char output_dir_path_buffer[1024] = "";
    char output_extension_buffer[16] = ".mkv";
    void showOutputDirHeader() {
        if(ImGui::CollapsingHeader("Output")) {
            ImGui::Text("Output Directory:  ");
            ImGui::SameLine();
            ImGui::InputText("##output_dir", output_dir_path_buffer, 1024);
            ImGui::SameLine();
            if(ImGui::Button("...##output_dir_btn")) {
                output_dir_dialog.OpenDialog("ChooseDirDlgKey",
                                             "Choose a Directory", nullptr,
                                             output_dir_path_buffer);
            }
            if(output_dir_dialog.Display("ChooseDirDlgKey")) {
                if(output_dir_dialog.IsOk()) {
                    std::string tmp_dir_path =
                        output_dir_dialog.GetFilePathName();
                    std::strcpy(output_dir_path_buffer, tmp_dir_path.c_str());
                }
                output_dir_dialog.Close();
            }

            ImGui::Text("Output Extension:  ");
            ImGui::SameLine();
            ImGui::InputText("##output_extension", output_extension_buffer, 16);
        }
    }

    void showParameters(ImVec2 pos, ImVec2 size) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Transcode Parameters", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        showSourceDirHeader();
        showVideoHeader();
        showAudioHeader();
        showOutputDirHeader();

        ImGui::End();
    }
    void showControlPanel(ImVec2 pos, ImVec2 size) {
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Transcode Control Panel", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        if(ImGui::Button("Transcode")) {
            scan();
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
            ImGui::Text("Transcoding: %ld / %ld",
                        std::distance(files.cbegin(), file_iterator),
                        files.size());
        }

        ImGui::End();
    }
    void show(ImVec2 pos, ImVec2 size) {
        showParameters(pos, ImVec2(size.x, size.y - 100));
        showControlPanel(ImVec2(pos.x, pos.y + size.y - 100),
                         ImVec2(size.x, 100));
        transcode();
    };
    const char * name() const { return "Transcode"; }
};

}  // namespace ImFFmpeg

#endif  // IMFFMPEG_TRANSCODE_GUI_HPP