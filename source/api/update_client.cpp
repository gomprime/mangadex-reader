#include "api/update_client.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <vector>

#include "json.hpp"
#include "net/http_client.hpp"
#include "util/app_path.hpp"

using json = nlohmann::json;

namespace api
{

namespace
{
    constexpr const char* kReleasesUrl = "https://api.github.com/repos/gomprime/mangadex-reader/releases/latest";

    std::vector<int> parseVersion(const std::string& version)
    {
        std::vector<int> parts;
        std::stringstream ss(version);
        std::string segment;

        while (std::getline(ss, segment, '.'))
        {
            try
            {
                parts.push_back(std::stoi(segment));
            }
            catch (const std::exception&)
            {
                parts.push_back(0);
            }
        }

        return parts;
    }
}

UpdateInfo UpdateClient::CheckLatestRelease()
{
    UpdateInfo info;

    // GitHub's API requires a non-empty, non-generic User-Agent (already
    // sent by HttpClient for every request) - no auth needed for a public
    // repo's releases.
    net::HttpResponse response = net::HttpClient::Get(kReleasesUrl, { "Accept: application/vnd.github+json" });

    if (!response.ok)
    {
        info.errorDetail = response.curlError.empty() ? ("HTTP " + std::to_string(response.status)) : response.curlError;
        return info;
    }

    try
    {
        json body = json::parse(response.body);

        std::string tag = body.value("tag_name", "");
        if (!tag.empty() && tag[0] == 'v')
            tag = tag.substr(1);
        info.latestVersion = tag;

        info.htmlUrl = body.value("html_url", "");

        if (body.contains("assets") && body["assets"].is_array())
        {
            for (const auto& asset : body["assets"])
            {
                std::string name = asset.value("name", "");
                if (name.size() >= 4 && name.compare(name.size() - 4, 4, ".nro") == 0)
                {
                    info.downloadUrl = asset.value("browser_download_url", "");
                    break;
                }
            }
        }

        info.ok = true;
    }
    catch (const json::exception& e)
    {
        info.errorDetail = e.what();
    }

    return info;
}

bool UpdateClient::IsNewerVersion(const std::string& current, const std::string& latest)
{
    std::vector<int> currentParts = parseVersion(current);
    std::vector<int> latestParts = parseVersion(latest);

    size_t count = std::max(currentParts.size(), latestParts.size());
    for (size_t i = 0; i < count; i++)
    {
        int a = i < latestParts.size() ? latestParts[i] : 0;
        int b = i < currentParts.size() ? currentParts[i] : 0;

        if (a != b)
            return a > b;
    }

    return false;
}

bool UpdateClient::DownloadAndInstall(const std::string& downloadUrl, std::string& errorOut)
{
    const std::string& nroPath = util::getAppNroPath();
    if (nroPath.empty())
    {
        errorOut = "unknown install path";
        return false;
    }

    std::string tmpPath = nroPath + ".tmp";

    if (!net::HttpClient::DownloadToFile(downloadUrl, tmpPath))
    {
        errorOut = "download failed";
        remove(tmpPath.c_str());
        return false;
    }

    // Unlike POSIX, the Switch's sdmc devoptab rejects rename() if the
    // destination already exists instead of overwriting it. Move the old
    // .nro aside first (itself a rename, so it can't fail on that same
    // ground) instead of deleting it outright, so a failed swap below still
    // leaves a working install rather than none at all.
    std::string bakPath = nroPath + ".bak";
    remove(bakPath.c_str());
    bool hadExisting = rename(nroPath.c_str(), bakPath.c_str()) == 0;

    if (rename(tmpPath.c_str(), nroPath.c_str()) != 0)
    {
        errorOut = std::string("couldn't replace the installed file (") + strerror(errno) + ")";
        remove(tmpPath.c_str());
        if (hadExisting)
            rename(bakPath.c_str(), nroPath.c_str());
        return false;
    }

    if (hadExisting)
        remove(bakPath.c_str());

    return true;
}

} // namespace api
