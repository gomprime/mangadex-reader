#pragma once

#include <string>
#include <vector>

namespace storage
{

struct LibraryEntry
{
    std::string mangaId;
    std::string title;
    std::string coverFileName;
    std::string lastReadChapterId;
    std::string lastReadChapterNumber;
    int lastReadPage = 0;
    long long updatedAt = 0; // unix seconds, used for sorting
};

struct Settings
{
    std::vector<std::string> contentRatings { "safe", "suggestive" };
    std::string imageQuality = "data-saver"; // "data" or "data-saver"
    int cacheLimitMB = 500;
    std::string preferredLanguage = "en";
};

// Reads/writes sdmc:/switch/mangadex-reader/library.json and settings.json.
// Not thread-safe by design: only ever call from the main thread, since it's
// driven by UI actions (adding a favorite, saving reading progress on exit).
class LibraryStore
{
  public:
    static std::vector<LibraryEntry> LoadLibrary();
    static void SaveLibrary(const std::vector<LibraryEntry>& entries);

    // Inserts or updates the entry for mangaId, bumping updatedAt to now.
    static void Upsert(const LibraryEntry& entry);
    static void Remove(const std::string& mangaId);
    static bool Contains(const std::string& mangaId);

    static Settings LoadSettings();
    static void SaveSettings(const Settings& settings);
};

} // namespace storage
