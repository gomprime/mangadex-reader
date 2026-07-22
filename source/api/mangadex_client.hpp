#pragma once

#include <string>

#include "api/models.hpp"

namespace api
{

enum class ImageQuality
{
    Data, // original quality
    DataSaver, // compressed
};

// Thin, stateless wrapper around the public (no-auth) MangaDex REST endpoints.
// Every method performs a blocking network call - always call from a worker thread.
class MangaDexClient
{
  public:
    // GET /manga?title=&includes[]=cover_art&order[relevance]=desc
    // language: MangaDex availableTranslatedLanguage code (e.g. "pt-br"), or
    // empty to not filter by language at all.
    static SearchResult SearchManga(const std::string& title, int offset, int limit,
        const std::vector<std::string>& contentRatings, const std::string& language = "");

    // GET /manga?order[latestUploadedChapter]=desc&includes[]=cover_art
    static SearchResult GetLatestUpdates(int offset, int limit, const std::vector<std::string>& contentRatings,
        const std::string& language = "");

    // GET /manga?order[followedCount]=desc&includes[]=cover_art
    static SearchResult GetPopular(int offset, int limit, const std::vector<std::string>& contentRatings,
        const std::string& language = "");

    // GET /manga/{id}?includes[]=cover_art
    static bool GetMangaDetail(const std::string& mangaId, Manga& out);

    // GET /manga/{id}/feed?translatedLanguage[]=&order[chapter]=asc
    static ChapterFeed GetChapterFeed(const std::string& mangaId, const std::string& language, int offset, int limit);

    // GET /at-home/server/{chapterId}, resolved into final page image URLs.
    static AtHomeServer GetAtHomeServer(const std::string& chapterId, ImageQuality quality);

    // Pure URL builder, no network call. size is "256" or "512" (thumbnail) or empty (full size).
    static std::string CoverUrl(const std::string& mangaId, const std::string& coverFileName,
        const std::string& size = "256");
};

} // namespace api
