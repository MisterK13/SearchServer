#pragma once

#include <vector>
#include <string>
#include <future>
#include <memory>
#include "search_server.h"
#include "thread_pool.h"

class AsyncSearchServer
{
public:
    explicit AsyncSearchServer(const SearchServer &server, size_t num_threads = 0);
    ~AsyncSearchServer() = default;

    // Асинхронный поиск с предикатом
    template <typename DocumentPredicate>
    std::future<std::vector<Document>> FindTopDocumentsAsync(
        const std::string &raw_query,
        DocumentPredicate document_predicate) const;

    // Асинхронный поиск по статусу
    std::future<std::vector<Document>> FindTopDocumentsAsync(
        const std::string &raw_query,
        DocumentStatus status) const;

    // Асинхронный поиск (стандартный)
    std::future<std::vector<Document>> FindTopDocumentsAsync(
        const std::string &raw_query) const;

    // Пакетная обработка нескольких запросов
    std::vector<std::future<std::vector<Document>>> FindTopDocumentsBatch(
        const std::vector<std::string> &queries) const;

    // Пакетная обработка запросов со статусом
    std::vector<std::future<std::vector<Document>>> FindTopDocumentsBatch(
        const std::vector<std::string> &queries,
        DocumentStatus status) const;

    size_t GetThreadCount() const { return pool_.GetThreadCount(); }
    size_t GetPendingTasks() const { return pool_.GetPendingTasks(); }

private:
    const SearchServer &server_;
    mutable ThreadPool pool_;
};

template <typename DocumentPredicate>
std::future<std::vector<Document>> AsyncSearchServer::FindTopDocumentsAsync(
    const std::string &raw_query,
    DocumentPredicate document_predicate) const
{
    return pool_.Enqueue([this, raw_query, document_predicate]()
                         { return server_.FindTopDocuments(raw_query, document_predicate); });
}