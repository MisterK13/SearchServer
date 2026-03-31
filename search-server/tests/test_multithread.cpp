#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <atomic>
#include "search_server.h"

using namespace std;

void TestConcurrentReads()
{
    cout << "Test 1: Concurrent reads..." << endl;

    SearchServer server("a the");
    server.AddDocument(1, "cat dog mouse", DocumentStatus::ACTUAL, {5, 4, 5});
    server.AddDocument(2, "cat cat cat", DocumentStatus::ACTUAL, {3});
    server.AddDocument(3, "dog dog", DocumentStatus::ACTUAL, {4, 4});

    const int num_threads = 10;
    const int iterations_per_thread = 100;

    vector<thread> threads;
    atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&server, &success_count, i]()
                             {
            for (int j = 0; j < iterations_per_thread; ++j) {
                try {
                    auto docs = server.FindTopDocuments("cat");
                    if (!docs.empty()) {
                        success_count++;
                    }
                    
                    auto match_result = server.MatchDocument("cat", 1);
                    if (!get<0>(match_result).empty()) {
                        success_count++;
                    }
                    
                    int count = server.GetDocumentCount();
                    assert(count == 3);
                } catch (const exception& e) {
                    cerr << "Thread " << i << " error: " << e.what() << endl;
                }
            } });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    cout << "  Success count: " << success_count << endl;
    cout << "  Test passed!" << endl;
}

void TestConcurrentReadsAndWrites()
{
    cout << "Test 2: Concurrent reads and writes..." << endl;

    SearchServer server("a the");

    for (int i = 1; i <= 10; ++i)
    {
        server.AddDocument(i, "initial document", DocumentStatus::ACTUAL, {5});
    }

    const int num_readers = 5;
    const int num_writers = 2;
    const int operations_per_thread = 50;

    vector<thread> threads;
    atomic<int> documents_added{0};

    for (int i = 0; i < num_readers; ++i)
    {
        threads.emplace_back([&server, i, operations_per_thread]()
                             {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    auto docs = server.FindTopDocuments("initial");
                } catch (const exception& e) {
                    cerr << "Reader " << i << " error: " << e.what() << endl;
                }
            } });
    }

    for (int i = 0; i < num_writers; ++i)
    {
        threads.emplace_back([&server, &documents_added, i, operations_per_thread]()
                             {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    int doc_id = 100 + i * operations_per_thread + j;
                    server.AddDocument(doc_id, "new document added", 
                                      DocumentStatus::ACTUAL, {3, 4});
                    documents_added++;
                } catch (const exception& e) {
                    cerr << "Writer " << i << " error: " << e.what() << endl;
                }
            } });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    int final_count = server.GetDocumentCount();
    cout << "  Documents added: " << documents_added << endl;
    cout << "  Final document count: " << final_count << endl;
    cout << "  Expected: " << (10 + documents_added) << endl;
    assert(final_count == 10 + documents_added);
    cout << "  Test passed!" << endl;
}

void TestConcurrentFindTopDocuments()
{
    cout << "Test 3: Concurrent FindTopDocuments with different queries..." << endl;

    SearchServer server("a the in on");

    server.AddDocument(1, "cat dog mouse", DocumentStatus::ACTUAL, {5});
    server.AddDocument(2, "cat cat cat", DocumentStatus::ACTUAL, {4});
    server.AddDocument(3, "dog dog dog", DocumentStatus::ACTUAL, {3});
    server.AddDocument(4, "mouse mouse", DocumentStatus::ACTUAL, {5});
    server.AddDocument(5, "cat dog", DocumentStatus::ACTUAL, {4});

    vector<string> queries = {
        "cat",
        "dog",
        "mouse",
        "cat dog",
        "cat -dog",
        "dog -cat"};

    const int num_threads = 20;
    vector<thread> threads;
    atomic<int> successful_searches{0};

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&server, &queries, &successful_searches, i]()
                             {
            for (int j = 0; j < 100; ++j) {
                const auto& query = queries[static_cast<size_t>(j) % queries.size()];
                try {
                    auto docs = server.FindTopDocuments(query);
                    if (!docs.empty()) {
                        successful_searches++;
                    }
                    
                    for (size_t k = 1; k < docs.size(); ++k) {
                        assert(docs[k-1].relevance >= docs[k].relevance);
                    }
                } catch (const exception& e) {
                    cerr << "Thread " << i << " error: " << e.what() << endl;
                }
            } });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    cout << "  Successful searches: " << successful_searches << endl;
    cout << "  Test passed!" << endl;
}

int main()
{
    cout << "Starting multithreading tests..." << endl;
    cout << "=================================" << endl;

    try
    {
        TestConcurrentReads();
        cout << endl;

        TestConcurrentReadsAndWrites();
        cout << endl;

        TestConcurrentFindTopDocuments();
        cout << endl;

        cout << "=================================" << endl;
        cout << "All tests passed successfully!" << endl;
    }
    catch (const exception &e)
    {
        cerr << "Test failed with exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
