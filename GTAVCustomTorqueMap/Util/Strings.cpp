#include "Strings.h"

#include <algorithm>

// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

std::vector<std::string> StrUtil::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    StrUtil::split(s, delim, std::back_inserter(elems));
    return elems;
}

std::string StrUtil::to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string StrUtil::replace_chars(std::string s, std::string replaceChars, char replacement) {
    for (auto it = s.begin(); it < s.end(); ++it) {
        if (replaceChars.find(*it) != std::string::npos) {
            *it = replacement;
        }
    }
    return s;
}

// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring/25385766
bool StrUtil::loose_match(std::string a, std::string b) {
    trim(a);
    trim(b);
    return to_lower(a) == to_lower(b);
}
