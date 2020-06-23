#include "Strings.h"

#include <algorithm>

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
