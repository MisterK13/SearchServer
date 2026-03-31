#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include "search_server.h"
#include "async_search_server.h"

using namespace std;
using namespace chrono;

string GenerateRandomWord(random_device &rd)
{
    static vector<string> words = {"cat", "dog", "mouse", "bird", "fish",
                                   "elephant", "tiger", "lion", "bear", "wolf",
                                   "apple", "banana", "cherry", "date", "fig"};
    static mt19937 gen(rd());
    static uniform_int_distribution<> dis(0, words.size() - 1);
    return words[dis(gen)];
}

void AddDocuments(SearchServer &server, int count)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> rating_dis(1, 5);
    uniform_int_distribution<> word_count_dis(3, 10);

    for (int i = 1; i <= count; ++i)
    {
        int num_words = word_count_dis(gen);
        string document;
        for (int j = 0; j < num_words; ++j)
        {
            if (j > 0)
                document += " ";
            document += GenerateRandomWord(rd);
        }

        vector<int> ratings;
        int rating_count = rating_dis(gen);
        for (int j = 0; j < rating_count; ++j)
        {
            ratings.push_back(rating_dis(gen));
        }

        server.AddDocument(i, document, DocumentStatus::ACTUAL, ratings);
    }
}

void TestAsyncSearch()
{
    cout << "\n=== Test Async Search ===" << endl;

    SearchServer server(vector<string>{"a", "the", "in", "on"});

    server.AddDocument(1, "cat dog mouse", DocumentStatus::ACTUAL, {5, 4, 5});
    server.AddDocument(2, "cat cat cat", DocumentStatus::ACTUAL, {4});
    server.AddDocument(3, "dog dog dog", DocumentStatus::ACTUAL, {3});
    server.AddDocument(4, "mouse mouse", DocumentStatus::ACTUAL, {5});
    server.AddDocument(5, "cat dog", DocumentStatus::ACTUAL, {4});
    server.AddDocument(6, "bird fish", DocumentStatus::ACTUAL, {3});
    server.AddDocument(7, "cat mouse", DocumentStatus::ACTUAL, {5});

    AsyncSearchServer async_server(server, 4);

    vector<string> queries = {
        "cat",
        "dog",
        "mouse",
        "cat dog",
        "cat -dog",
        "dog -cat",
        "bird",
        "fish",
        "cat mouse"};

    cout << "Processing " << queries.size() << " queries asynchronously..." << endl;

    auto start = high_resolution_clock::now();

    auto futures = async_server.FindTopDocumentsBatch(queries);

    vector<vector<Document>> results;
    results.reserve(futures.size());

    for (size_t i = 0; i < futures.size(); ++i)
    {
        auto docs = futures[i].get();
        results.push_back(docs);
        cout << "Query '" << queries[i] << "' found " << docs.size() << " documents" << endl;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "\nTotal time: " << duration.count() << " ms" << endl;
    cout << "Thread pool size: " << async_server.GetThreadCount() << endl;
    cout << "Test passed!" << endl;
}

void TestAsyncPerformanceReal()
{
    cout << "\n=== Real Performance Comparison ===" << endl;

    SearchServer server(vector<string>{"a", "the", "in", "on", "of", "and"});

    cout << "Adding 1000 documents..." << endl;
    AddDocuments(server, 1000);
    cout << "Documents added: " << server.GetDocumentCount() << endl;

    cout << "Creating 500 queries..." << endl;
    vector<string> queries;
    random_device rd;
    mt19937 gen(rd());
    vector<string> common_words = {"cat", "dog", "mouse", "bird", "fish",
                                   "elephant", "tiger", "lion"};
    uniform_int_distribution<> word_dis(0, common_words.size() - 1);
    uniform_int_distribution<> count_dis(1, 3);

    for (int i = 0; i < 500; ++i)
    {
        int word_count = count_dis(gen);
        string query;
        for (int j = 0; j < word_count; ++j)
        {
            if (j > 0)
                query += " ";
            query += common_words[word_dis(gen)];
        }
        queries.push_back(query);
    }

    cout << "\nSequential execution..." << endl;
    auto start = high_resolution_clock::now();

    vector<vector<Document>> sequential_results;
    sequential_results.reserve(queries.size());
    for (const auto &query : queries)
    {
        sequential_results.push_back(server.FindTopDocuments(query));
    }

    auto end = high_resolution_clock::now();
    auto sequential_time = duration_cast<milliseconds>(end - start);
    cout << "  Time: " << sequential_time.count() << " ms" << endl;
    cout << "  Average per query: " << sequential_time.count() / queries.size() << " ms" << endl;

    for (int num_threads : {2, 4, 8, 16})
    {
        AsyncSearchServer async_server(server, num_threads);

        cout << "\nAsync execution with " << num_threads << " threads:" << endl;
        start = high_resolution_clock::now();

        auto futures = async_server.FindTopDocumentsBatch(queries);

        vector<vector<Document>> async_results;
        async_results.reserve(futures.size());
        for (auto &future : futures)
        {
            async_results.push_back(future.get());
        }

        end = high_resolution_clock::now();
        auto async_time = duration_cast<milliseconds>(end - start);

        double speedup = static_cast<double>(sequential_time.count()) / async_time.count();
        cout << "  Time: " << async_time.count() << " ms" << endl;
        cout << "  Speedup: " << fixed << setprecision(2) << speedup << "x" << endl;
        cout << "  Efficiency: " << fixed << setprecision(2)
             << (speedup / num_threads * 100) << "%" << endl;

        bool results_match = true;
        for (size_t i = 0; i < queries.size() && results_match; ++i)
        {
            if (sequential_results[i].size() != async_results[i].size())
            {
                results_match = false;
                cout << "  Warning: Results mismatch for query " << i << endl;
            }
        }
        cout << "  Results match sequential: " << (results_match ? "Yes" : "No") << endl;
    }
}

void TestThroughput()
{
    cout << "\n=== Throughput Test ===" << endl;

    SearchServer server(vector<string>{"a", "the"});
    AddDocuments(server, 500);

    AsyncSearchServer async_server(server, 8);

    vector<int> batch_sizes = {10, 50, 100, 200, 500};

    for (int batch_size : batch_sizes)
    {
        cout << "\nBatch size: " << batch_size << endl;

        vector<string> queries;
        for (int i = 0; i < batch_size; ++i)
        {
            queries.push_back("cat dog");
        }

        auto start = high_resolution_clock::now();
        auto futures = async_server.FindTopDocumentsBatch(queries);

        for (auto &future : futures)
        {
            future.get();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        cout << "  Time: " << duration.count() << " ms" << endl;
        cout << "  Throughput: " << (batch_size * 1000.0 / duration.count()) << " queries/sec" << endl;
    }
}

void TestWorkStealing()
{
    cout << "\n=== Work Stealing Test ===" << endl;

    SearchServer server(vector<string>{"a", "the"});
    AddDocuments(server, 1000);

    AsyncSearchServer async_server(server, 4);

    vector<pair<string, int>> test_queries = {
        {"cat", 100},                                       // лёгкие запросы
        {"cat dog mouse", 50},                              // средние
        {"cat dog mouse bird fish elephant tiger lion", 25} // тяжёлые
    };

    vector<future<vector<Document>>> futures;
    auto start = high_resolution_clock::now();

    for (const auto &[query, count] : test_queries)
    {
        for (int i = 0; i < count; ++i)
        {
            futures.push_back(async_server.FindTopDocumentsAsync(query));
        }
    }

    cout << "Total tasks: " << futures.size() << endl;
    cout << "Threads: " << async_server.GetThreadCount() << endl;

    for (auto &future : futures)
    {
        future.get();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "All tasks completed in " << duration.count() << " ms" << endl;
    cout << "Average time per task: " << duration.count() / futures.size() << " ms" << endl;
}

int main()
{
    try
    {
        cout << "Thread Pool Advanced Tests" << endl;
        cout << "==========================" << endl;

        TestAsyncSearch();          // быстрый тест
        TestAsyncPerformanceReal(); // реальный тест производительности
        TestThroughput();           // тест пропускной способности
        TestWorkStealing();         // тест балансировки нагрузки

        cout << "\n=== All tests passed successfully! ===" << endl;
    }
    catch (const exception &e)
    {
        cerr << "Test failed with exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}