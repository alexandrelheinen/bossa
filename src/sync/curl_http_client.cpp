/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bossa/sync/curl_http_client.hpp"

#include <curl/curl.h>

namespace bossa::sync {

namespace {

std::size_t write_callback(char *pointer, std::size_t size, std::size_t nmemb,
                           void *userdata) {
    auto *body = static_cast<std::string *>(userdata);
    body->append(pointer, size * nmemb);
    return size * nmemb;
}

} // namespace

CurlHttpClient::CurlHttpClient() { curl_global_init(CURL_GLOBAL_DEFAULT); }

CurlHttpClient::~CurlHttpClient() { curl_global_cleanup(); }

HttpResponse
CurlHttpClient::post(const std::string &url,
                     const std::map<std::string, std::string> &headers,
                     const std::string &body, int timeout_seconds) {
    HttpResponse response;
    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        response.error = "curl_easy_init failed";
        return response;
    }

    struct curl_slist *header_list = nullptr;
    header_list =
        curl_slist_append(header_list, "Content-Type: application/json");
    for (const auto &[key, value] : headers) {
        const std::string line = key + ": " + value;
        header_list = curl_slist_append(header_list, line.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    const CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        response.error = curl_easy_strerror(code);
    } else {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        response.status_code = static_cast<int>(status);
    }

    curl_slist_free_all(header_list);
    curl_easy_cleanup(curl);
    return response;
}

} // namespace bossa::sync
