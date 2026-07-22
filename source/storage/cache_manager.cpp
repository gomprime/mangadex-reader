#include "storage/cache_manager.hpp"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "net/http_client.hpp"
#include "storage/paths.hpp"

namespace storage
{

namespace
{
    struct CachedFile
    {
        std::string path;
        long long mtime;
        long long size;
    };

    bool fileExists(const std::string& path)
    {
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }

    // Recursively collects every regular file under dirPath.
    void collectFiles(const std::string& dirPath, std::vector<CachedFile>& out)
    {
        DIR* dir = opendir(dirPath.c_str());
        if (!dir)
            return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;

            std::string fullPath = dirPath + "/" + name;

            struct stat st;
            if (stat(fullPath.c_str(), &st) != 0)
                continue;

            if (S_ISDIR(st.st_mode))
                collectFiles(fullPath, out);
            else
                out.push_back({ fullPath, static_cast<long long>(st.st_mtime), static_cast<long long>(st.st_size) });
        }

        closedir(dir);
    }

    // Deletes every file in dirPath, then the (now empty) subdirectories.
    void clearDirectoryRecursive(const std::string& dirPath)
    {
        DIR* dir = opendir(dirPath.c_str());
        if (!dir)
            return;

        std::vector<std::string> subdirs;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;

            std::string fullPath = dirPath + "/" + name;

            struct stat st;
            if (stat(fullPath.c_str(), &st) != 0)
                continue;

            if (S_ISDIR(st.st_mode))
                subdirs.push_back(fullPath);
            else
                remove(fullPath.c_str());
        }

        closedir(dir);

        for (const auto& subdir : subdirs)
        {
            clearDirectoryRecursive(subdir);
            rmdir(subdir.c_str());
        }
    }
}

std::string CacheManager::GetOrDownloadCover(const std::string& mangaId, const std::string& coverUrl)
{
    if (coverUrl.empty())
        return "";

    std::string path = CoverCachePath(mangaId);
    if (fileExists(path))
        return path;

    if (net::HttpClient::DownloadToFile(coverUrl, path))
        return path;

    return "";
}

std::string CacheManager::GetOrDownloadPage(const std::string& chapterId, int pageIndex, const std::string& pageUrl)
{
    if (pageUrl.empty())
        return "";

    std::string dir = ChapterCacheDir(chapterId);
    mkdir(dir.c_str(), 0777);

    char indexPrefix[8];
    snprintf(indexPrefix, sizeof(indexPrefix), "%03d_", pageIndex);

    std::string path = dir + "/" + indexPrefix + FileNameFromUrl(pageUrl);
    if (fileExists(path))
        return path;

    if (net::HttpClient::DownloadToFile(pageUrl, path))
        return path;

    return "";
}

void CacheManager::EnforceCacheLimit(int limitMB)
{
    long long limitBytes = static_cast<long long>(limitMB) * 1024 * 1024;

    std::vector<CachedFile> files;
    collectFiles(kCoversDir, files);
    collectFiles(kPagesDir, files);

    long long totalSize = 0;
    for (const auto& file : files)
        totalSize += file.size;

    if (totalSize <= limitBytes)
        return;

    // Oldest modified first.
    std::sort(files.begin(), files.end(), [](const CachedFile& a, const CachedFile& b) {
        return a.mtime < b.mtime;
    });

    for (const auto& file : files)
    {
        if (totalSize <= limitBytes)
            break;

        if (remove(file.path.c_str()) == 0)
            totalSize -= file.size;
    }
}

void CacheManager::ClearAll()
{
    clearDirectoryRecursive(kCoversDir);
    clearDirectoryRecursive(kPagesDir);
}

} // namespace storage
