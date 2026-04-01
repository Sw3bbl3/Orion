#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "orion/AudioEngine.h"

namespace Orion {

    struct ProjectInfo {
        std::string name;
        std::string path;
        std::string date;
        std::string version;
        std::string sizeString;
        std::string thumbnailPath;
        bool isFavorite;


        std::string author;
        std::string genre;
        int bpm = 120;
        int sampleRate = 44100;
        int bitDepth = 32;
    };

    class ProjectManager {
    public:
        ProjectManager();


        std::vector<ProjectInfo> getAllProjects();
        std::vector<ProjectInfo> getRecentProjects();
        std::vector<ProjectInfo> getFavoriteProjects();


        bool createProject(AudioEngine& engine, const std::string& name, const std::string& templateName, int bpm, int sampleRate, const std::string& author = "", const std::string& genre = "", const std::string& parentPath = "");
        bool duplicateProject(const std::string& path);
        bool renameProject(const std::string& path, const std::string& newName);
        bool deleteProject(const std::string& path, bool* deletedFolder = nullptr, std::string* errorMessage = nullptr);
        void toggleFavorite(const std::string& path);
        bool isFavorite(const std::string& path);
        void addToRecents(const std::string& path);

    private:
        std::string getFormattedDate(std::filesystem::file_time_type time);
        std::string getProjectVersion(const std::string& path);
        std::string getProjectSize(const std::string& folderPath);
    };

}
