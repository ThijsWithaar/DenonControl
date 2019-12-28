#pragma once

#include <functional>
#include <string_view>

bool startswith(std::string_view data, std::string_view key);

std::vector<std::string_view> split(std::string_view strv, std::string_view delims = " ");

void splitlines(std::string_view data, std::function<void(std::string_view)> cb);

std::pair<std::string_view, std::string_view> splitKeyVal(std::string_view line);

std::string urlDecode(std::string_view data);
