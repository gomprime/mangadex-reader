#pragma once

#include <string>
#include <vector>

namespace api
{

struct Manga
{
    std::string id;
    std::string title;
    std::string description;
    std::string status; // "ongoing", "completed", "hiatus", "cancelled"
    std::string contentRating; // "safe", "suggestive", "erotica", "pornographic"
    std::string coverFileName; // relationship attribute, empty if none
    std::vector<std::string> tags;
};

struct SearchResult
{
    bool ok = false; // false means the request itself failed (network/TLS/HTTP error)
    long httpStatus = 0;
    std::string errorDetail; // libcurl error string, set when httpStatus stays 0
    std::vector<Manga> items;
    int total = 0;
};

struct Chapter
{
    std::string id;
    std::string chapterNumber; // string on purpose: MangaDex allows "3.5" etc, and null -> ""
    std::string volume;
    std::string title;
    std::string translatedLanguage;
    int pageCount = 0;
    std::string publishAt;
    // Non-empty when this chapter isn't hosted on MangaDex at all - it's a
    // pointer to an official/licensed reader elsewhere (e.g. MangaPlus).
    // /at-home/server has no page data to serve for these.
    std::string externalUrl;
};

struct ChapterFeed
{
    bool ok = false; // false means the request itself failed (network/TLS/HTTP error)
    long httpStatus = 0;
    std::string errorDetail; // libcurl error string, set when httpStatus stays 0
    std::vector<Chapter> items;
    int total = 0;
};

struct AtHomeServer
{
    bool ok = false;
    std::vector<std::string> pageUrls;
};

} // namespace api
