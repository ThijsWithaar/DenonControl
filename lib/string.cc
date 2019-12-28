#include <Denon/string.h>

#include <cstring>
#include <iostream>


bool startswith(std::string_view data, std::string_view key)
{
	return data.compare(0, key.size(), key) == 0;
}


std::vector<std::string_view> split(std::string_view strv, std::string_view delims)
{
	std::vector<std::string_view> output;
	size_t first = 0;
	while (first < strv.size())
	{
		const auto second = strv.find_first_of(delims, first);
		if (first != second)
			output.emplace_back(strv.substr(first, second-first));
		if (second == std::string_view::npos)
			break;
		first = second + 1;
	}
	return output;
}


void splitlines(std::string_view data, std::function<void(std::string_view)> cb)
{
	size_t nPrev = 0;
	bool prevEol = false;
	for(size_t n=0; n<data.size(); n++)
	{
		bool eol = data[n] == '\r' || data[n] == '\n';
		if(!prevEol && eol)
			cb(data.substr(nPrev, n-nPrev));
		if(!eol && prevEol)
			nPrev = n;		// Start of new line
		prevEol = eol;
	}
}


std::pair<std::string_view, std::string_view> splitKeyVal(std::string_view line)
{
	auto sep = line.find(':');
	if(sep == std::string_view::npos)
		return {};
	auto key = line.substr(0, sep);
	auto val = line.substr(sep+2);
	return {key, val};
}


std::string urlDecode(std::string_view data)
{
	using pair = std::pair<std::string, char>;
	const std::vector<pair> pairs = {
		{"lt", '<'},
		{"gt", '>'},
		{"quot", '"'},
		{"amp", '&'},
		{"apos", '\''},
	};

	std::string dec;
	size_t idx = 0;
	while(idx < data.size())
	{
		if(data[idx] == '&')
		{
			auto pair = std::find_if(pairs.begin(), pairs.end(), [&data, idx](auto pair)
			{
				return std::memcmp(pair.first.data(), &data[idx] + 1, pair.first.size()) == 0;
			});
			if(pair != pairs.end())
			{
				dec.push_back(pair->second);
				idx += pair->first.size() + 2;
			}
			else
			{
				//std::cout << "urlDecode: not found " << data.substr(idx, 10) << "\n";
				dec.push_back(data[idx++]);
			}
		}
		else
			dec.push_back(data[idx++]);
	}

	return dec;
}
