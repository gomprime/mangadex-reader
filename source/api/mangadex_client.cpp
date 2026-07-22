#include "api/mangadex_client.hpp"

#include <curl/curl.h>

#include "json.hpp"
#include "net/http_client.hpp"

using json = nlohmann::json;

namespace api
{

namespace
{
    constexpr const char* kApiBase = "https://api.mangadex.org";
    constexpr const char* kUploadsBase = "https://uploads.mangadex.org";

    std::string urlEncode(const std::string& value)
    {
        CURL* curl = curl_easy_init();
        if (!curl)
            return value;

        char* escaped = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
        std::string result = escaped ? escaped : value;

        if (escaped)
            curl_free(escaped);
        curl_easy_cleanup(curl);

        return result;
    }

    // Many MangaDex string attributes (chapter title, volume, chapter
    // number...) are present-but-null rather than absent when unset (most
    // chapters have no individual title, for instance). json::value(key,
    // default) only substitutes the default for a *missing* key - a
    // present null still gets passed to get<string>(), which throws
    // type_error.302. That exception used to propagate out of the calling
    // parse function entirely, wiping out an otherwise-successful response
    // (including its real HTTP status) and reporting a misleading
    // "HTTP 0" with no detail instead of whatever the actual fetch result
    // was.
    std::string safeString(const json& obj, const char* key, const std::string& fallback = "")
    {
        if (!obj.is_object() || !obj.contains(key) || !obj[key].is_string())
            return fallback;
        return obj[key].get<std::string>();
    }

    // MangaDex localized string objects are {"en": "...", "ja": "...", ...}.
    // Prefer English, fall back to whatever is first available.
    std::string pickLocalizedString(const json& localized)
    {
        if (!localized.is_object() || localized.empty())
            return "";

        if (localized.contains("en") && localized["en"].is_string())
            return localized["en"].get<std::string>();

        for (auto& [key, value] : localized.items())
        {
            if (value.is_string())
                return value.get<std::string>();
        }

        return "";
    }

    Manga parseMangaEntry(const json& entry)
    {
        Manga manga;
        manga.id = safeString(entry, "id");

        const json& attributes = entry.value("attributes", json::object());
        manga.title = pickLocalizedString(attributes.value("title", json::object()));
        manga.description = pickLocalizedString(attributes.value("description", json::object()));
        manga.status = safeString(attributes, "status");
        manga.contentRating = safeString(attributes, "contentRating", "safe");

        if (attributes.contains("tags") && attributes["tags"].is_array())
        {
            for (const auto& tag : attributes["tags"])
            {
                std::string name = pickLocalizedString(tag.value("attributes", json::object()).value("name", json::object()));
                if (!name.empty())
                    manga.tags.push_back(name);
            }
        }

        if (entry.contains("relationships") && entry["relationships"].is_array())
        {
            for (const auto& rel : entry["relationships"])
            {
                if (safeString(rel, "type") == "cover_art" && rel.contains("attributes"))
                {
                    manga.coverFileName = safeString(rel["attributes"], "fileName");
                    break;
                }
            }
        }

        return manga;
    }

    SearchResult parseMangaCollection(const json& body)
    {
        SearchResult result;
        result.ok = true;
        result.total = body.value("total", 0);

        if (body.contains("data") && body["data"].is_array())
        {
            for (const auto& entry : body["data"])
                result.items.push_back(parseMangaEntry(entry));
        }

        return result;
    }

    std::string buildContentRatingQuery(const std::vector<std::string>& contentRatings)
    {
        std::string query;
        for (const auto& rating : contentRatings)
            query += "&contentRating[]=" + urlEncode(rating);
        return query;
    }

    std::string buildLanguageQuery(const std::string& language)
    {
        if (language.empty())
            return "";
        return "&availableTranslatedLanguage[]=" + urlEncode(language);
    }

    SearchResult fetchMangaCollection(const std::string& query)
    {
        net::globalRateLimiter().acquire();

        std::string url = std::string(kApiBase) + "/manga?includes[]=cover_art&limit=" + query;
        net::HttpResponse response = net::HttpClient::Get(url);
        if (!response.ok)
        {
            SearchResult result;
            result.httpStatus = response.status;
            result.errorDetail = response.curlError;
            return result;
        }

        try
        {
            SearchResult result = parseMangaCollection(json::parse(response.body));
            result.httpStatus = response.status;
            return result;
        }
        catch (const json::exception&)
        {
            SearchResult result;
            result.httpStatus = response.status;
            return result;
        }
    }
}

SearchResult MangaDexClient::SearchManga(const std::string& title, int offset, int limit,
    const std::vector<std::string>& contentRatings, const std::string& language)
{
    std::string query = std::to_string(limit) + "&offset=" + std::to_string(offset)
        + "&title=" + urlEncode(title) + "&order[relevance]=desc" + buildContentRatingQuery(contentRatings)
        + buildLanguageQuery(language);
    return fetchMangaCollection(query);
}

SearchResult MangaDexClient::GetLatestUpdates(int offset, int limit, const std::vector<std::string>& contentRatings,
    const std::string& language)
{
    std::string query = std::to_string(limit) + "&offset=" + std::to_string(offset)
        + "&order[latestUploadedChapter]=desc" + buildContentRatingQuery(contentRatings) + buildLanguageQuery(language);
    return fetchMangaCollection(query);
}

SearchResult MangaDexClient::GetPopular(int offset, int limit, const std::vector<std::string>& contentRatings,
    const std::string& language)
{
    std::string query = std::to_string(limit) + "&offset=" + std::to_string(offset)
        + "&order[followedCount]=desc" + buildContentRatingQuery(contentRatings) + buildLanguageQuery(language);
    return fetchMangaCollection(query);
}

bool MangaDexClient::GetMangaDetail(const std::string& mangaId, Manga& out)
{
    net::globalRateLimiter().acquire();

    std::string url = std::string(kApiBase) + "/manga/" + urlEncode(mangaId) + "?includes[]=cover_art";
    net::HttpResponse response = net::HttpClient::Get(url);
    if (!response.ok)
        return false;

    try
    {
        json body = json::parse(response.body);
        if (!body.contains("data"))
            return false;

        out = parseMangaEntry(body["data"]);
        return true;
    }
    catch (const json::exception&)
    {
        return false;
    }
}

ChapterFeed MangaDexClient::GetChapterFeed(const std::string& mangaId, const std::string& language, int offset, int limit)
{
    net::globalRateLimiter().acquire();

    std::string url = std::string(kApiBase) + "/manga/" + urlEncode(mangaId) + "/feed?limit=" + std::to_string(limit)
        + "&offset=" + std::to_string(offset) + "&order[volume]=asc&order[chapter]=asc";
    if (!language.empty())
        url += "&translatedLanguage[]=" + urlEncode(language);

    net::HttpResponse response = net::HttpClient::Get(url);

    ChapterFeed feed;
    feed.httpStatus = response.status;
    if (!response.ok)
    {
        feed.errorDetail = response.curlError;
        return feed;
    }

    try
    {
        json body = json::parse(response.body);
        feed.ok = true;
        feed.total = body.value("total", 0);

        if (!body.contains("data") || !body["data"].is_array())
            return feed;

        for (const auto& entry : body["data"])
        {
            const json& attributes = entry.value("attributes", json::object());

            Chapter chapter;
            chapter.id = safeString(entry, "id");
            chapter.chapterNumber = safeString(attributes, "chapter");
            chapter.volume = safeString(attributes, "volume");
            chapter.title = safeString(attributes, "title");
            chapter.translatedLanguage = safeString(attributes, "translatedLanguage");
            chapter.pageCount = attributes.value("pages", 0);
            chapter.publishAt = safeString(attributes, "publishAt");
            chapter.externalUrl = safeString(attributes, "externalUrl");

            feed.items.push_back(std::move(chapter));
        }
    }
    catch (const json::exception&)
    {
        return {};
    }

    return feed;
}

AtHomeServer MangaDexClient::GetAtHomeServer(const std::string& chapterId, ImageQuality quality)
{
    net::atHomeRateLimiter().acquire();

    std::string url = std::string(kApiBase) + "/at-home/server/" + urlEncode(chapterId);
    net::HttpResponse response = net::HttpClient::Get(url);

    AtHomeServer result;
    if (!response.ok)
        return result;

    try
    {
        json body = json::parse(response.body);
        std::string baseUrl = safeString(body, "baseUrl");
        const json& chapter = body.value("chapter", json::object());
        std::string hash = safeString(chapter, "hash");

        const char* qualityDir = (quality == ImageQuality::Data) ? "data" : "data-saver";
        const char* fileListKey = (quality == ImageQuality::Data) ? "data" : "dataSaver";

        if (baseUrl.empty() || hash.empty() || !chapter.contains(fileListKey))
            return result;

        for (const auto& fileName : chapter[fileListKey])
        {
            result.pageUrls.push_back(baseUrl + "/" + qualityDir + "/" + hash + "/" + fileName.get<std::string>());
        }

        result.ok = !result.pageUrls.empty();
    }
    catch (const json::exception&)
    {
        return {};
    }

    return result;
}

std::string MangaDexClient::CoverUrl(const std::string& mangaId, const std::string& coverFileName, const std::string& size)
{
    if (coverFileName.empty())
        return "";

    std::string url = std::string(kUploadsBase) + "/covers/" + mangaId + "/" + coverFileName;
    if (!size.empty())
        url += "." + size + ".jpg";

    return url;
}

} // namespace api
