#ifndef IMFFMPEG_ALGORITHM_GUI_HPP
#define IMFFMPEG_ALGORITHM_GUI_HPP

#include <imgui.h>

#include "utils/imvec_operators.hpp"

namespace ImFFmpeg {

class CommandGUI {
public:
    virtual void show(ImVec2 pos, ImVec2 size) = 0;
    virtual const char * name() const = 0;
};

}  // namespace ImFFmpeg

#endif  // ImFFmpeg_ALGORITHM_GUI_HPP