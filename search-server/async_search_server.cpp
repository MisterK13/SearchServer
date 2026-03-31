#include "async_search_server.h"

AsyncSearchServer::AsyncSearchServer(const SearchServer &server, size_t num_threads)
    : server_(server), pool_(num_threads ? num_threads : std::thread::hardware_concurrency())
{
}

std::future<std::vector<Document>> AsyncSearchServer::FindTopDocumentsAsync(
    const std::string &raw_query,
    DocumentStatus status) const
{
    return pool_.Enqueue([this, raw_query, status]()
                         { return server_.FindTopDocuments(raw_query, status); });
}

std::future<std::vector<Document>> AsyncSearchServer::FindTopDocumentsAsync(
    const std::string &raw_query) const
{
    return pool_.Enqueue([this, raw_query]()
                         { return server_.FindTopDocuments(raw_query); });
}

std::vector<std::future<std::vector<Document>>> AsyncSearchServer::FindTopDocumentsBatch(
    const std::vector<std::string> &queries) const
{
    std::vector<std::future<std::vector<Document>>> futures;
    futures.reserve(queries.size());
    for (const auto &query : queries)
    {
        futures.push_back(FindTopDocumentsAsync(query));
    }
    return futures;
}

std::vector<std::future<std::vector<Document>>> AsyncSearchServer::FindTopDocumentsBatch(
    const std::vector<std::string> &queries,
    DocumentStatus status) const
{
    std::vector<std::future<std::vector<Document>>> futures;
    futures.reserve(queries.size());
    for (const auto &query : queries)
    {
        futures.push_back(FindTopDocumentsAsync(query, status));
    }
    return futures;
}