#include "util/app_path.hpp"

namespace util
{

namespace
{
    std::string g_nroPath;
}

void setAppNroPath(const std::string& path)
{
    g_nroPath = path;
}

const std::string& getAppNroPath()
{
    return g_nroPath;
}

} // namespace util
