#pragma once

#include <functional>
#include <sstream>
#include <string_view>


template<typename T>
std::string toString(T v)
{
	std::stringstream ss;
	ss << v;
	return ss.str();
};

bool startswith(std::string_view data, std::string_view key);

std::vector<std::string_view> split(std::string_view strv, std::string_view delims = " ");

void splitlines(std::string_view data, std::function<void(std::string_view)> cb);

std::pair<std::string_view, std::string_view> splitKeyVal(std::string_view line);

std::string xmlDecode(std::string_view data);
