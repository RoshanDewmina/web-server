// utils.hpp

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <unordered_map>

// Utility function to check if a string ends with a given suffix
bool ends_with(const std::string& str, const std::string& suffix);

// Function to get MIME type based on file extension
std::string get_mime_type(const std::string& path);

// Function to parse HTTP requests, updated to return unordered_map
std::unordered_map<std::string, std::string> parse_request(const std::string& request);

#endif // UTILS_HPP
