#include "ProjectManager.h"
#include "orion/SettingsManager.h"
#include "orion/ProjectSerializer.h"
#include "orion/Logger.h"
#include <fstream>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

namespace Orion {

    namespace {
        std::string normalizePathForCompare(const std::filesystem::path& p)
        {
            auto normalized = p.lexically_normal().generic_string();
#ifdef _WIN32
            std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
#endif
            return normalized;
        }

        bool isPathWithin(const std::filesystem::path& candidate, const std::filesystem::path& parent)
        {
            auto candidateNorm = normalizePathForCompare(candidate);
            auto parentNorm = normalizePathForCompare(parent);
            if (parentNorm.empty()) return false;
            if (candidateNorm.size() < parentNorm.size()) return false;
            if (candidateNorm.compare(0, parentNorm.size(), parentNorm) != 0) return false;
            return candidateNorm.size() == parentNorm.size()
                   || candidateNorm[parentNorm.size()] == '/'
                   || candidateNorm[parentNorm.size()] == '\\';
        }

        bool eraseProjectReferencesFromRecents(const std::string& projectPath, const std::filesystem::path& deletedFolder)
        {
            if (!std::filesystem::exists("recents.json")) return false;

            std::vector<std::string> recents;
            try {
                std::ifstream in("recents.json");
                nlohmann::json j;
                in >> j;
                recents = j.get<std::vector<std::string>>();
            } catch (...) {
                return false;
            }

            const auto projectNorm = normalizePathForCompare(projectPath);
            const bool folderDeleted = !deletedFolder.empty();

            auto oldSize = recents.size();
            recents.erase(std::remove_if(recents.begin(), recents.end(), [&](const std::string& recentPath) {
                if (normalizePathForCompare(recentPath) == projectNorm) return true;
                if (!folderDeleted) return false;
                return isPathWithin(std::filesystem::path(recentPath), deletedFolder);
            }), recents.end());

            if (recents.size() == oldSize) return false;

            try {
                std::ofstream out("recents.json");
                out << nlohmann::json(recents).dump(4);
                return true;
            } catch (...) {
                return false;
            }
        }
    }

    ProjectManager::ProjectManager() {

        auto root = SettingsManager::get().rootDataPath;
        if (!root.empty()) {
            std::filesystem::create_directories(std::filesystem::path(root) / "Projects");
        }
    }

    std::string ProjectManager::getFormattedDate(std::filesystem::file_time_type time) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
        std::tm tm = *std::localtime(&tt);
        char buffer[32];
        std::strftime(buffer, 32, "%b %d, %Y", &tm);
        return std::string(buffer);
    }


    static void fillProjectMetadata(const std::string& path, ProjectInfo& info) {
        try {
            std::ifstream f(path);
            if (!f.is_open()) return;
            nlohmann::json j;
            f >> j;

            if (j.contains("version")) info.version = j["version"];
            if (j.contains("bpm")) info.bpm = j["bpm"];
            if (j.contains("sampleRate")) info.sampleRate = j["sampleRate"];
            if (j.contains("author")) info.author = j["author"];
            if (j.contains("genre")) info.genre = j["genre"];

        } catch(...) {

        }
    }

    std::string ProjectManager::getProjectSize(const std::string& folderPath) {
        uint64_t size = 0;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
                if (!entry.is_directory()) {
                    size += entry.file_size();
                }
            }
        } catch(...) { return "Unknown"; }

        if (size < 1024) return std::to_string(size) + " B";
        if (size < 1024 * 1024) return std::to_string(size / 1024) + " KB";
        if (size < 1024 * 1024 * 1024) return std::to_string(size / (1024 * 1024)) + " MB";
        return std::to_string(size / (1024 * 1024 * 1024)) + " GB";
    }

    bool ProjectManager::isFavorite(const std::string& path) {
        const auto& favs = SettingsManager::get().favorites;
        return std::find(favs.begin(), favs.end(), path) != favs.end();
    }

    void ProjectManager::toggleFavorite(const std::string& path) {
        auto settings = SettingsManager::get();
        auto it = std::find(settings.favorites.begin(), settings.favorites.end(), path);
        if (it != settings.favorites.end()) {
            settings.favorites.erase(it);
        } else {
            settings.favorites.push_back(path);
        }
        SettingsManager::set(settings);
    }

    void ProjectManager::addToRecents(const std::string& path) {
        std::vector<std::string> recents;

        if (std::filesystem::exists("recents.json")) {
            try {
                std::ifstream f("recents.json");
                nlohmann::json j;
                f >> j;
                recents = j.get<std::vector<std::string>>();
            } catch(...) {}
        }


        recents.erase(std::remove(recents.begin(), recents.end(), path), recents.end());

        recents.insert(recents.begin(), path);

        if (recents.size() > 20) recents.resize(20);


        try {
            std::ofstream f("recents.json");
            nlohmann::json j = recents;
            f << j.dump(4);
        } catch(...) {}
    }

    std::vector<ProjectInfo> ProjectManager::getAllProjects() {
        std::vector<ProjectInfo> projects;
        std::string root = SettingsManager::get().rootDataPath;
        if (root.empty()) return projects;

        std::filesystem::path projectsDir = std::filesystem::path(root) / "Projects";
        if (!std::filesystem::exists(projectsDir)) return projects;

        for (const auto& entry : std::filesystem::directory_iterator(projectsDir)) {
            if (entry.is_directory()) {

                std::filesystem::path projectFile;
                bool found = false;


                std::string expectedName = entry.path().filename().string() + ".orion";
                if (std::filesystem::exists(entry.path() / expectedName)) {
                    projectFile = entry.path() / expectedName;
                    found = true;
                } else {

                    for (const auto& subEntry : std::filesystem::directory_iterator(entry.path())) {
                        if (subEntry.is_regular_file() && subEntry.path().extension() == ".orion") {
                            projectFile = subEntry.path();
                            found = true;
                            break;
                        }
                    }
                }

                if (found) {
                    ProjectInfo info;
                    info.name = entry.path().filename().string();
                    info.path = projectFile.string();
                    info.date = getFormattedDate(std::filesystem::last_write_time(projectFile));
                    info.isFavorite = isFavorite(info.path);
                    info.sizeString = getProjectSize(entry.path().string());
                    info.version = "1.0.0";

                    fillProjectMetadata(info.path, info);

                    std::filesystem::path thumb = entry.path() / "thumbnail.png";
                    if (std::filesystem::exists(thumb)) info.thumbnailPath = thumb.string();

                    projects.push_back(info);
                }
            }
        }


        std::sort(projects.begin(), projects.end(), [](const ProjectInfo& a, const ProjectInfo& b) {
            return a.date > b.date;
        });

        return projects;
    }

    std::vector<ProjectInfo> ProjectManager::getRecentProjects() {
        std::vector<ProjectInfo> projects;
        std::vector<std::string> recentPaths;

        if (std::filesystem::exists("recents.json")) {
            try {
                std::ifstream f("recents.json");
                nlohmann::json j;
                f >> j;
                recentPaths = j.get<std::vector<std::string>>();
            } catch(...) {}
        }

        for (const auto& path : recentPaths) {
            if (std::filesystem::exists(path)) {
                ProjectInfo info;
                std::filesystem::path p(path);
                info.name = p.stem().string();
                info.path = path;
                info.date = getFormattedDate(std::filesystem::last_write_time(path));
                info.isFavorite = isFavorite(path);
                info.sizeString = getProjectSize(p.parent_path().string());
                info.version = "1.0.0";

                fillProjectMetadata(info.path, info);

                std::filesystem::path thumb = p.parent_path() / "thumbnail.png";
                if (std::filesystem::exists(thumb)) info.thumbnailPath = thumb.string();

                projects.push_back(info);
            }
        }
        return projects;
    }

    std::vector<ProjectInfo> ProjectManager::getFavoriteProjects() {
        std::vector<ProjectInfo> projects;
        const auto& favs = SettingsManager::get().favorites;

        for (const auto& path : favs) {
             if (std::filesystem::exists(path)) {
                ProjectInfo info;
                std::filesystem::path p(path);
                info.name = p.stem().string();
                info.path = path;
                info.date = getFormattedDate(std::filesystem::last_write_time(path));
                info.isFavorite = true;
                info.sizeString = getProjectSize(p.parent_path().string());
                info.version = "1.0.0";

                fillProjectMetadata(info.path, info);

                std::filesystem::path thumb = p.parent_path() / "thumbnail.png";
                if (std::filesystem::exists(thumb)) info.thumbnailPath = thumb.string();

                projects.push_back(info);
            }
        }
        return projects;
    }

    bool ProjectManager::duplicateProject(const std::string& path) {
        try {
            std::filesystem::path projectFile(path);
            std::filesystem::path projectDir = projectFile.parent_path();
            std::filesystem::path projectsParent = projectDir.parent_path();

            std::string newName = projectDir.filename().string() + " Copy";
            std::filesystem::path newDir = projectsParent / newName;

            int i = 1;
            while (std::filesystem::exists(newDir)) {
                newDir = projectsParent / (newName + " " + std::to_string(++i));
            }

            std::filesystem::copy(projectDir, newDir, std::filesystem::copy_options::recursive);

            // Rename the .orion file inside
            std::filesystem::path oldOrion = newDir / projectFile.filename();
            std::filesystem::path newOrion = newDir / (newDir.filename().string() + ".orion");
            std::filesystem::rename(oldOrion, newOrion);

            return true;
        } catch (...) {
            return false;
        }
    }

    bool ProjectManager::renameProject(const std::string& path, const std::string& newName) {
        try {
            std::filesystem::path projectFile(path);
            std::filesystem::path projectDir = projectFile.parent_path();
            std::filesystem::path projectsParent = projectDir.parent_path();

            std::filesystem::path newDir = projectsParent / newName;
            if (std::filesystem::exists(newDir)) return false;

            std::filesystem::rename(projectDir, newDir);

            // Rename the .orion file inside
            std::filesystem::path oldOrion = newDir / projectFile.filename();
            std::filesystem::path newOrion = newDir / (newName + ".orion");
            std::filesystem::rename(oldOrion, newOrion);

            return true;
        } catch (...) {
            return false;
        }
    }

    bool ProjectManager::deleteProject(const std::string& path, bool* deletedFolder, std::string* errorMessage) {
        if (deletedFolder) *deletedFolder = false;
        if (errorMessage) errorMessage->clear();

        auto fail = [&](const std::string& message) {
            if (errorMessage) *errorMessage = message;
            return false;
        };

        try {
            std::filesystem::path projectFile(path);
            if (projectFile.empty()) {
                return fail("Invalid project path.");
            }

            if (!std::filesystem::exists(projectFile)) {
                return fail("Project file does not exist.");
            }

            if (projectFile.extension() != ".orion") {
                return fail("Only .orion project files can be deleted.");
            }

            const auto projectDir = projectFile.parent_path();
            const bool canDeleteFolder =
                !projectDir.empty()
                && projectDir.filename().string() == projectFile.stem().string();

            std::error_code ec;
            std::filesystem::path removedFolderPath;

            if (canDeleteFolder) {
                const auto removed = std::filesystem::remove_all(projectDir, ec);
                if (ec || removed == 0) {
                    return fail("Failed to delete project folder: " + projectDir.string());
                }
                if (deletedFolder) *deletedFolder = true;
                removedFolderPath = projectDir;
            } else {
                if (!std::filesystem::remove(projectFile, ec) || ec) {
                    return fail("Failed to delete project file: " + projectFile.string());
                }
            }

            auto settings = SettingsManager::get();
            const auto projectNorm = normalizePathForCompare(projectFile);
            const bool folderDeleted = !removedFolderPath.empty();
            auto favoritesOldSize = settings.favorites.size();

            settings.favorites.erase(std::remove_if(settings.favorites.begin(), settings.favorites.end(),
                [&](const std::string& favoritePath) {
                    if (normalizePathForCompare(favoritePath) == projectNorm) return true;
                    if (!folderDeleted) return false;
                    return isPathWithin(std::filesystem::path(favoritePath), removedFolderPath);
                }),
                settings.favorites.end());

            if (settings.favorites.size() != favoritesOldSize) {
                SettingsManager::set(settings);
            }

            eraseProjectReferencesFromRecents(projectFile.string(), removedFolderPath);
            return true;
        } catch (const std::exception& e) {
            return fail(std::string("Delete failed: ") + e.what());
        } catch (...) {
            return fail("Delete failed due to an unknown error.");
        }
    }

    bool ProjectManager::createProject(AudioEngine& engine, const std::string& name, const std::string& templateName, int bpm, int sampleRate, const std::string& author, const std::string& genre, const std::string& parentPath) {
        std::filesystem::path projectDir;
        if (!parentPath.empty()) {
             projectDir = std::filesystem::path(parentPath) / name;
        } else {
             std::string root = SettingsManager::get().rootDataPath;
             if (root.empty()) return false;
             projectDir = std::filesystem::path(root) / "Projects" / name;
        }

        if (std::filesystem::exists(projectDir)) {

            return false;
        }

        std::filesystem::create_directories(projectDir);


        std::filesystem::create_directories(projectDir / "Audio");
        std::filesystem::create_directories(projectDir / "Bounces");
        std::filesystem::create_directories(projectDir / "Exports");
        std::filesystem::create_directories(projectDir / "Backups");

        std::string projectPath = (projectDir / (name + ".orion")).string();


        engine.clearTracks();
        engine.setBpm((float)bpm);
        engine.setSampleRate(sampleRate);
        engine.setAuthor(author);
        engine.setGenre(genre);


        if (templateName == "Basic Beat") {


        }


        ProjectSerializer::save(engine, projectPath);


        addToRecents(projectPath);

        return true;
    }

}
