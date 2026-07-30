#pragma once

#include <string>

namespace api
{

struct UpdateInfo
{
    bool ok = false; // false means the request itself failed (network/HTTP error)
    std::string errorDetail;
    std::string latestVersion; // e.g. "1.0.1" (leading "v" already stripped)
    std::string downloadUrl; // direct .nro asset URL, empty if the release has none
    std::string htmlUrl; // release page, for reference
};

// Thin client for the one GitHub endpoint we need: the latest release of
// this project's own repo. Used to offer an in-app update instead of making
// people check GitHub manually.
class UpdateClient
{
  public:
    static UpdateInfo CheckLatestRelease();

    // true if `latest` is a newer version than `current` (both "X.Y.Z").
    static bool IsNewerVersion(const std::string& current, const std::string& latest);

    // Downloads downloadUrl over the currently running .nro (util::getAppNroPath())
    // via a temp file + rename so a failed/interrupted download can't corrupt
    // it. Must be called from a worker thread.
    static bool DownloadAndInstall(const std::string& downloadUrl, std::string& errorOut);
};

} // namespace api
