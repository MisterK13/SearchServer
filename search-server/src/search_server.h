#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <shared_mutex>

#include "string_processing.h"
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer
{
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(const std::string &stop_words_text);

    void AddDocument(int document_id, const std::string &document, DocumentStatus status,
                     const std::vector<int> &ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string &raw_query,
                                           DocumentPredicate document_predicate) const
    {
        std::shared_lock lock(mutex_);
        return FindTopDocumentsNoLock(raw_query, document_predicate);
    }

    std::vector<Document> FindTopDocuments(const std::string &raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string &raw_query) const;

    int GetDocumentCount() const;
    int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string &raw_query,
                                                                       int document_id) const;

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> documents_index_;

    mutable std::shared_mutex mutex_;

    bool IsStopWord(const std::string &word) const;
    std::vector<std::string> SplitIntoWordsNoStop(const std::string &text) const;
    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord
    {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    struct Query
    {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string &text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string &word) const;

    static bool IsValidWord(const std::string &word);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocumentsNoLock(const std::string &raw_query,
                                                 DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocumentsNoLock(const Query &query,
                                                 DocumentPredicate document_predicate) const;

    int GetDocumentCountNoLock() const
    {
        return static_cast<int>(documents_.size());
    }
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    using namespace std;
    for (const auto &word : stop_words_)
    {
        if (!IsValidWord(word))
        {
            throw invalid_argument("Стоп слова содержат недопустимые символы");
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocumentsNoLock(const std::string &raw_query,
                                                           DocumentPredicate document_predicate) const
{
    using namespace std;
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocumentsNoLock(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs)
         {
             return lhs.relevance > rhs.relevance || ((std::abs(lhs.relevance - rhs.relevance) < EPSILON) &&
                                                      lhs.rating > rhs.rating);
         });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocumentsNoLock(const Query &query,
                                                           DocumentPredicate document_predicate) const
{
    using namespace std;
    map<int, double> document_to_relevance;

    for (const string &word : query.plus_words)
    {
        auto it = word_to_document_freqs_.find(word);
        if (it == word_to_document_freqs_.end())
        {
            continue;
        }

        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        for (const auto &[document_id, term_freq] : it->second)
        {
            const auto &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const string &word : query.minus_words)
    {
        auto it = word_to_document_freqs_.find(word);
        if (it == word_to_document_freqs_.end())
        {
            continue;
        }
        for (const auto &[document_id, _] : it->second)
        {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto &[document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}