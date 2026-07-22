#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace net
{

struct HttpResponse
{
    bool ok = false;
    long status = 0;
    std::string body;
    std::string curlError; // human-readable libcurl error, set when status stays 0 (pre-HTTP failure)
};

// Simple sliding-window rate limiter: blocks the calling thread (never the
// main thread - only call this from worker threads) until a slot is free.
// MangaDex enforces ~5 req/s globally and 40 req/min on /at-home/server.
class RateLimiter
{
  public:
    RateLimiter(int maxRequests, double perSeconds);
    void acquire();

  private:
    int maxRequests;
    double windowSeconds;
    std::vector<double> timestamps;
    std::mutex mutex;
};

// Global limiters shared by every MangaDex API call. Defined in http_client.cpp.
RateLimiter& globalRateLimiter();
RateLimiter& atHomeRateLimiter();

// Blocking HTTP client wrapping libcurl. Must be called from a worker thread,
// never the main/UI thread - requests can take hundreds of ms on a real network.
class HttpClient
{
  public:
    static HttpResponse Get(const std::string& url, const std::vector<std::string>& headers = {});

    // Streams the response body directly to disk, avoiding loading large
    // chapter page images fully into RAM. Returns true on HTTP 2xx and a
    // successful write.
    static bool DownloadToFile(const std::string& url, const std::string& destPath,
        const std::vector<std::string>& headers = {});
};

} // namespace net
