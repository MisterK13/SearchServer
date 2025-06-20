#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

SearchServer::SearchServer(const std::string &stop_words_text)
    : SearchServer(
          SplitIntoWords(stop_words_text)) // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string &document, DocumentStatus status,
                               const std::vector<int> &ratings)
{
    using namespace std;
    if (document_id < 0)
    {
        throw invalid_argument("Попытка добавить документ с отрицательным id");
    }

    if (documents_.count(document_id))
    {
        throw invalid_argument("Попытка добавить документ c id ранее добавленного документа");
    }

    documents_index_.push_back(document_id);
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();

    for (const string &word : words)
    {
        if (!IsValidWord(word))
        {
            throw invalid_argument("Наличие недопустимых символов (с кодами от 0 до 31) в тексте добавляемого документа");
        }

        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query, DocumentStatus status) const
{
    using namespace std;
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
    ;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query) const
{
    using namespace std;
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string &raw_query,
                                                                                 int document_id) const
{
    using namespace std;
    const Query query = ParseQuery(raw_query);
    vector<string> matched_words;
    for (const string &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }

        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
        }
    }

    for (const string &word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }

        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.clear();
            break;
        }
    }

    const tuple<vector<string>, DocumentStatus> match_documents = {matched_words, documents_.at(document_id).status};
    return match_documents;
}

int SearchServer::GetDocumentId(int index) const
{
    using namespace std;

    if (index >= 0 && index < static_cast<int>(documents_index_.size()))
    {
        return documents_index_[index];
    }
    else
    {
        throw out_of_range("Индекс переданного документа выходит за пределы допустимого диапазона ");
    }
}

bool SearchServer::IsStopWord(const std::string &word) const
{
    using namespace std;
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string &text) const
{
    using namespace std;
    vector<string> words;
    for (const string &word : SplitIntoWords(text))
    {
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int> &ratings)
{
    using namespace std;
    if (ratings.empty())
    {
        return 0;
    }

    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
{
    using namespace std;
    bool is_minus = false;

    if (!IsValidWord(text))
    {
        throw invalid_argument("В словах поискового запроса есть недопустимые символы с кодами от 0 до 31");
    }
    // Word shouldn't be empty
    if (text[0] == '-')
    {
        if (text[1] == '-')
        {
            throw invalid_argument("Наличие более чем одного минуса перед словами");
        }
        if (text.size() == 1)
        {
            throw invalid_argument("Отсутствие текста после символа «минус» в поисковом запросе");
        }

        is_minus = true;
        text = text.substr(1);
    }

    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string &text) const
{
    using namespace std;
    Query query;
    for (const string &word : SplitIntoWords(text))
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.insert(query_word.data);
            }
            else
            {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string &word) const
{
    using namespace std;
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string &word)
{
    using namespace std;
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c)
                   { return c >= '\0' && c < ' '; });
}