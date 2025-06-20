#pragma once

#include <deque>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer &search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string &raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        QueryResult(const bool x) : check_empty(x) {}
        bool check_empty = false;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer &search_server_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate)
{
    const auto documents_found = search_server_.FindTopDocuments(raw_query, document_predicate);
    QueryResult empty_request(documents_found.empty());
    if (requests_.size() <= min_in_day_)
    {
        requests_.push_back(empty_request);
    }

    if (requests_.size() > min_in_day_)
    {
        requests_.push_back(empty_request);
        requests_.pop_front();
    }

    return documents_found;
}
