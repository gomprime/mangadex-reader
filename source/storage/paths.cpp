#include "storage/paths.hpp"

#include <sys/stat.h>

namespace storage
{

namespace
{
    void makeDir(const std::string& path)
    {
        mkdir(path.c_str(), 0777); // ignore errors: EEXIST is the common/expected case
    }
}

void EnsureDirectories()
{
    makeDir(kBaseDir);
    makeDir(std::string(kBaseDir) + "/cache");
    makeDir(kCoversDir);
    makeDir(kPagesDir);
}

std::string FileNameFromUrl(const std::string& url)
{
    size_t pos = url.find_last_of('/');
    if (pos == std::string::npos)
        return url;
    return url.substr(pos + 1);
}

} // namespace storage
