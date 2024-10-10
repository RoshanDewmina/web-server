// utils.cpp

#include "utils.hpp"
#include <unordered_map>
#include <sstream>

bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.length() >= suffix.length()) {
        return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
    } else {
        return false;
    }
}

std::string get_mime_type(const std::string& path) {
    if (ends_with(path, ".html") || ends_with(path, ".htm"))
        return "text/html";
    else if (ends_with(path, ".css"))
        return "text/css";
    else if (ends_with(path, ".js"))
        return "application/javascript";
    else if (ends_with(path, ".json"))
        return "application/json";
    else if (ends_with(path, ".png"))
        return "image/png";
    else if (ends_with(path, ".jpg") || ends_with(path, ".jpeg"))
        return "image/jpeg";
    else if (ends_with(path, ".gif"))
        return "image/gif";
    else
        return "application/octet-stream";
}

std::unordered_map<std::string, std::string> parse_request(const std::string& request) {
    std::unordered_map<std::string, std::string> request_info;
    std::istringstream stream(request);
    std::string line;

    // Parse request line
    if (std::getline(stream, line)) {
        std::istringstream request_line(line);
        request_line >> request_info["method"] >> request_info["path"] >> request_info["version"];
    }

    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        size_t delimiter = line.find(':');
        if (delimiter != std::string::npos) {
            std::string key = line.substr(0, delimiter);
            std::string value = line.substr(delimiter + 1);
            value.erase(0, value.find_first_not_of(" \r\n"));
            request_info[key] = value;
        }
    }

    // Parse body if any
    if (request_info.find("Content-Length") != request_info.end()) {
        int content_length = std::stoi(request_info["Content-Length"]);
        std::string body(content_length, ' ');
        stream.read(&body[0], content_length);
        request_info["body"] = body;
    }

    return request_info;
}
