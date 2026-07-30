#pragma once

#include <string>

namespace util
{

// Captured from argv[0] in main() - the full sdmc:/ path hbmenu launched
// this .nro from (e.g. "sdmc:/switch/mangadex-reader/mangadex-reader.nro").
// Needed by the self-updater to know what file to overwrite.
void setAppNroPath(const std::string& path);
const std::string& getAppNroPath();

} // namespace util
