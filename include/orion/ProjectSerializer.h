#pragma once

#include "AudioEngine.h"
#include <string>

namespace Orion {

    class ProjectSerializer {
    public:


        static bool save(const AudioEngine& engine, const std::string& filepath, const std::string& extraData = "");



        static bool load(AudioEngine& engine, const std::string& filepath, std::string* extraData = nullptr);
    };

}
