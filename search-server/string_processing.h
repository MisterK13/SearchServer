#pragma once
#include <string>
#include <set>
#include <vector>
#include <type_traits>

std::vector<std::string> SplitIntoWords(const std::string &text);

template <typename T>
std::string ConvertToString(const T &value)
{
    using namespace std;
    if constexpr (is_same_v<T, string>)
    {
        return value;
    }
    else if constexpr (is_same_v<T, const char *>)
    {
        return string(value);
    }
    else if constexpr (is_same_v<T, char>)
    {
        return string(1, value);
    }
    else
    {
        return string(value);
    }
}

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer &strings)
{
    using namespace std;
    set<string> non_empty_strings;

    for (const auto &str : strings)
    {
        string converted = ConvertToString(str);
        if (!converted.empty())
        {
            non_empty_strings.insert(converted);
        }
    }
    return non_empty_strings;
}

inline std::set<std::string> MakeUniqueNonEmptyStrings(const char *strings)
{
    return MakeUniqueNonEmptyStrings(std::string(strings));
}