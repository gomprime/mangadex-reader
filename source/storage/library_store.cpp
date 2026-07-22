#include "storage/library_store.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>

#include "json.hpp"
#include "storage/paths.hpp"

using json = nlohmann::json;

namespace storage
{

// Must be plain namespace-scope functions (not inside an anonymous namespace)
// so nlohmann::json's ADL-based serialization can find them.
void to_json(json& j, const LibraryEntry& e)
{
    j = json {
        { "mangaId", e.mangaId },
        { "title", e.title },
        { "coverFileName", e.coverFileName },
        { "lastReadChapterId", e.lastReadChapterId },
        { "lastReadChapterNumber", e.lastReadChapterNumber },
        { "lastReadPage", e.lastReadPage },
        { "updatedAt", e.updatedAt },
    };
}

void from_json(const json& j, LibraryEntry& e)
{
    e.mangaId = j.value("mangaId", "");
    e.title = j.value("title", "");
    e.coverFileName = j.value("coverFileName", "");
    e.lastReadChapterId = j.value("lastReadChapterId", "");
    e.lastReadChapterNumber = j.value("lastReadChapterNumber", "");
    e.lastReadPage = j.value("lastReadPage", 0);
    e.updatedAt = j.value("updatedAt", 0LL);
}

std::vector<LibraryEntry> LibraryStore::LoadLibrary()
{
    std::ifstream file(kLibraryFile);
    if (!file.is_open())
        return {};

    try
    {
        json body = json::parse(file, nullptr, false);
        if (body.is_discarded() || !body.is_array())
            return {};

        return body.get<std::vector<LibraryEntry>>();
    }
    catch (const json::exception&)
    {
        return {};
    }
}

void LibraryStore::SaveLibrary(const std::vector<LibraryEntry>& entries)
{
    std::ofstream file(kLibraryFile, std::ios::trunc);
    if (!file.is_open())
        return;

    json body = entries;
    file << body.dump(2);
}

void LibraryStore::Upsert(const LibraryEntry& entry)
{
    std::vector<LibraryEntry> entries = LoadLibrary();

    LibraryEntry updated = entry;
    updated.updatedAt = static_cast<long long>(time(nullptr));

    bool found = false;
    for (auto& existing : entries)
    {
        if (existing.mangaId == entry.mangaId)
        {
            existing = updated;
            found = true;
            break;
        }
    }

    if (!found)
        entries.push_back(updated);

    SaveLibrary(entries);
}

void LibraryStore::Remove(const std::string& mangaId)
{
    std::vector<LibraryEntry> entries = LoadLibrary();

    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const LibraryEntry& e) {
        return e.mangaId == mangaId;
    }),
        entries.end());

    SaveLibrary(entries);
}

bool LibraryStore::Contains(const std::string& mangaId)
{
    for (const auto& entry : LoadLibrary())
    {
        if (entry.mangaId == mangaId)
            return true;
    }
    return false;
}

Settings LibraryStore::LoadSettings()
{
    Settings settings;

    std::ifstream file(kSettingsFile);
    if (!file.is_open())
        return settings;

    try
    {
        json body = json::parse(file, nullptr, false);
        if (body.is_discarded())
            return settings;

        if (body.contains("contentRatings") && body["contentRatings"].is_array())
            settings.contentRatings = body["contentRatings"].get<std::vector<std::string>>();

        settings.imageQuality = body.value("imageQuality", settings.imageQuality);
        settings.cacheLimitMB = body.value("cacheLimitMB", settings.cacheLimitMB);
        settings.preferredLanguage = body.value("preferredLanguage", settings.preferredLanguage);
    }
    catch (const json::exception&)
    {
        return Settings {};
    }

    return settings;
}

void LibraryStore::SaveSettings(const Settings& settings)
{
    std::ofstream file(kSettingsFile, std::ios::trunc);
    if (!file.is_open())
        return;

    json body = {
        { "contentRatings", settings.contentRatings },
        { "imageQuality", settings.imageQuality },
        { "cacheLimitMB", settings.cacheLimitMB },
        { "preferredLanguage", settings.preferredLanguage },
    };

    file << body.dump(2);
}

} // namespace storage
