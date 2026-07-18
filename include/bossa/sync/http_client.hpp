/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <map>
#include <string>

namespace bossa::sync {

/** @brief Result of an HTTP request. */
struct HttpResponse {
    int status_code{0};
    std::string body;
    std::string error;
};

/**
 * @brief Minimal HTTP client interface (mockable in tests).
 */
class HttpClient {
  public:
    virtual ~HttpClient() = default;

    /**
     * @brief Perform an HTTP POST.
     * @param url Absolute URL.
     * @param headers Extra request headers.
     * @param body Request body.
     * @param timeout_seconds Request timeout.
     * @return Response with status or transport error.
     */
    virtual HttpResponse post(const std::string &url,
                              const std::map<std::string, std::string> &headers,
                              const std::string &body, int timeout_seconds) = 0;
};

} // namespace bossa::sync
