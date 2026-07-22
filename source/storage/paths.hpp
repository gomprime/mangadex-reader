#pragma once

#include <string>

namespace storage
{

constexpr const char* kBaseDir = "sdmc:/switch/mangadex-reader";
constexpr const char* kCoversDir = "sdmc:/switch/mangadex-reader/cache/covers";
constexpr const char* kPagesDir = "sdmc:/switch/mangadex-reader/cache/pages";
constexpr const char* kLibraryFile = "sdmc:/switch/mangadex-reader/library.json";
constexpr const char* kSettingsFile = "sdmc:/switch/mangadex-reader/settings.json";

// Creates the base/cache directory tree on the SD card. Safe to call every launch.
void EnsureDirectories();

inline std::string CoverCachePath(const std::string& mangaId)
{
    return std::string(kCoversDir) + "/" + mangaId + ".jpg";
}

inline std::string ChapterCacheDir(const std::string& chapterId)
{
    return std::string(kPagesDir) + "/" + chapterId;
}

// Returns the last path segment of a URL, used as the cached file's name
// so the original image extension (.jpg/.png) is preserved.
std::string FileNameFromUrl(const std::string& url);

} // namespace storage
