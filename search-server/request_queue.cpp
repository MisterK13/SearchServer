#include "request_queue.h"

#include <deque>
#include <string>
#include <vector>

RequestQueue::RequestQueue(const SearchServer &search_server) : search_server_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status)
{
    return RequestQueue::AddFindRequest(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query)
{
    return RequestQueue::AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const
{
    int empty_requests = 0;
    for (auto el : requests_)
    {
        if (el.check_empty == true)
        {
            ++empty_requests;
        }
    }
    return empty_requests;
}
