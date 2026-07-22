#include "net/http_client.hpp"

#include <curl/curl.h>

#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>

namespace net
{

namespace
{
    constexpr const char* kUserAgent = "SwitchMangaDexReader/0.1.0 (Nintendo Switch homebrew; local build)";
    constexpr const char* kCaInfoPath = "romfs:/cacert.pem";

    // Serializes every curl_easy_perform() call process-wide. Each call uses
    // its own CURL* handle (never shared across threads), which is normally
    // enough for libcurl's own thread-safety guarantees - but our TLS
    // backend is a from-source mbedTLS 3.6.2 build, and its entropy/RNG
    // context is process-global and not documented as safe under truly
    // concurrent handshakes. Worker requests used to be spread out enough
    // in practice that simultaneous handshakes were rare; a fixed pool of
    // worker threads pulling from a queue as fast as possible made genuine
    // concurrent perform() calls common, and intermittent "HTTP 0" failures
    // with no libcurl error string (consistent with corruption rather than
    // a real network/protocol error) showed up right after. This trades
    // network parallelism for safety until the backend's thread-safety is
    // confirmed one way or the other.
    std::mutex curlPerformMutex;

    // curl_easy_strerror() should always return descriptive text for a
    // known CURLcode, but always append the raw numeric code too - if the
    // string ever comes back empty (seen intermittently on-device with this
    // from-source curl/mbedTLS build, cause not yet confirmed), the number
    // alone is enough to identify which CURLcode actually happened.
    std::string describeCurlError(CURLcode result)
    {
        const char* text = curl_easy_strerror(result);
        std::string out = "curl ";
        out += std::to_string(static_cast<int>(result));
        if (text && text[0] != '\0')
        {
            out += ": ";
            out += text;
        }
        return out;
    }

    size_t writeToString(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto* out = static_cast<std::string*>(userdata);
        out->append(ptr, size * nmemb);
        return size * nmemb;
    }

    size_t writeToFile(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto* file = static_cast<FILE*>(userdata);
        return fwrite(ptr, size, nmemb, file);
    }

    curl_slist* buildHeaders(const std::vector<std::string>& headers)
    {
        curl_slist* list = nullptr;
        for (const auto& header : headers)
            list = curl_slist_append(list, header.c_str());
        return list;
    }

    void applyCommonOptions(CURL* curl, const std::string& url, curl_slist* headerList)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
        curl_easy_setopt(curl, CURLOPT_CAINFO, kCaInfoPath);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        if (headerList)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
}

RateLimiter::RateLimiter(int maxRequests, double perSeconds)
    : maxRequests(maxRequests)
    , windowSeconds(perSeconds)
{
}

void RateLimiter::acquire()
{
    using namespace std::chrono;

    for (;;)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);

            double now = duration<double>(steady_clock::now().time_since_epoch()).count();

            // Drop timestamps outside the sliding window.
            while (!timestamps.empty() && now - timestamps.front() > windowSeconds)
                timestamps.erase(timestamps.begin());

            if (static_cast<int>(timestamps.size()) < maxRequests)
            {
                timestamps.push_back(now);
                return;
            }
        }

        // Slot not free: release the lock (scope above) and retry shortly.
        std::this_thread::sleep_for(milliseconds(50));
    }
}

RateLimiter& globalRateLimiter()
{
    static RateLimiter limiter(5, 1.0);
    return limiter;
}

RateLimiter& atHomeRateLimiter()
{
    static RateLimiter limiter(40, 60.0);
    return limiter;
}

HttpResponse HttpClient::Get(const std::string& url, const std::vector<std::string>& headers)
{
    HttpResponse response;

    CURL* curl = curl_easy_init();
    if (!curl)
        return response;

    curl_slist* headerList = buildHeaders(headers);
    applyCommonOptions(curl, url, headerList);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    CURLcode result;
    {
        std::lock_guard<std::mutex> lock(curlPerformMutex);
        result = curl_easy_perform(curl);
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    response.status = status;
    response.ok = (result == CURLE_OK) && status >= 200 && status < 300;
    if (result != CURLE_OK)
        response.curlError = describeCurlError(result);
    else if (status == 0)
        response.curlError = "resposta HTTP vazia (status 0)";

    if (headerList)
        curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    return response;
}

bool HttpClient::DownloadToFile(const std::string& url, const std::string& destPath,
    const std::vector<std::string>& headers)
{
    FILE* file = fopen(destPath.c_str(), "wb");
    if (!file)
        return false;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        fclose(file);
        return false;
    }

    curl_slist* headerList = buildHeaders(headers);
    applyCommonOptions(curl, url, headerList);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    CURLcode result;
    {
        std::lock_guard<std::mutex> lock(curlPerformMutex);
        result = curl_easy_perform(curl);
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    if (headerList)
        curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);
    fclose(file);

    bool ok = (result == CURLE_OK) && status >= 200 && status < 300;
    if (!ok)
        remove(destPath.c_str());

    return ok;
}

} // namespace net
