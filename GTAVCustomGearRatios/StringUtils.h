#pragma once
#include "inc/types.h"
#include <algorithm>
#include <locale>

Hash joaat(std::string s);
std::string to_lower(std::string data);
// https://stackoverflow.com/questions/3152241/case-insensitive-stdstring-find
// templated version of my_equal so it could work with both char and wchar_t
template<typename charT>
struct my_equal {
    my_equal(const std::locale& loc) : loc_(loc) {}
    bool operator()(charT ch1, charT ch2) {
        return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
    }
private:
    const std::locale& loc_;
};

// find substring (case insensitive)
template<typename T>
size_t findSubstring(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
    typename T::const_iterator it = std::search(str1.begin(), str1.end(),
        str2.begin(), str2.end(), my_equal<typename T::value_type>(loc));
    if (it != str1.end()) return it - str1.begin();
    else return -1; // not found
}
