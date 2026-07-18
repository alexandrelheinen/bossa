/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include "bossa/sync/http_client.hpp"

namespace bossa::sync {

/**
 * @brief libcurl-backed HttpClient for production uploads.
 */
class CurlHttpClient : public HttpClient {
  public:
    CurlHttpClient();
    ~CurlHttpClient() override;

    HttpResponse post(const std::string &url,
                      const std::map<std::string, std::string> &headers,
                      const std::string &body, int timeout_seconds) override;
};

} // namespace bossa::sync
