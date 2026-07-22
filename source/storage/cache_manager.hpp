#pragma once

#include <string>

namespace storage
{

// All methods perform blocking network/disk I/O - call from a worker thread only.
class CacheManager
{
  public:
    // Returns the local sdmc: path to the manga's cover, downloading it first if not cached.
    // Empty string on failure.
    static std::string GetOrDownloadCover(const std::string& mangaId, const std::string& coverUrl);

    // Returns the local sdmc: path to a chapter page, downloading it first if not cached.
    // Empty string on failure.
    static std::string GetOrDownloadPage(const std::string& chapterId, int pageIndex, const std::string& pageUrl);

    // Walks the whole cache directory, and if its total size exceeds limitMB,
    // deletes the least-recently-modified files until it fits.
    static void EnforceCacheLimit(int limitMB);

    // Deletes every cached cover and page file.
    static void ClearAll();
};

} // namespace storage
