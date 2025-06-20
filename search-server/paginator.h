#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator begin, const Iterator end) : begin_(begin), end_(end), size_(distance(begin, end))
    {
    }

    auto begin() const
    {
        return begin_;
    }

    auto end() const
    {
        return end_;
    }

    auto size() const
    {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    std::size_t size_;
};

template <typename Iterator>
std::ostream &operator<<(std::ostream &output, const IteratorRange<Iterator> &range)
{
    using namespace std;
    for (Iterator i = range.begin(); i != range.end(); ++i)
    {
        output << *i;
    }
    return output;
}

template <typename Iterator>
class Paginator
{
public:
    Paginator(Iterator begin, Iterator end, std::size_t page_size)
    {
        using namespace std;
        for (size_t i = distance(begin, end); i > 0;)
        {
            const size_t current_size = min(page_size, i);
            const Iterator current_end = next(begin, current_size);
            pages_.push_back({begin, current_end});
            i -= current_size;
            begin = current_end;
        }
    }

    auto begin() const
    {
        return pages_.begin();
    }

    auto end() const
    {
        return pages_.end();
    }

    auto size() const
    {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container &c, std::size_t page_size)
{
    return Paginator(begin(c), end(c), page_size);
}