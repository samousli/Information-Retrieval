#ifndef QUERY_EXECUTOR
#define QUERY_EXECUTOR

#include "IndexStat.hpp"
#include "IndexVector.hpp"
#include "InvertedIndex.hpp"
#include "VectorOps.hpp"
#include "SysUtils.hpp"

#include <unordered_map>
#include <map>
#include <vector>
#include <sstream>
#include <limits>
#include <set>
#include <algorithm>

typedef uint_fast64_t ulong;


#define PARALLELIZE_QUERY_EXEC 1

using namespace std;


template <typename A, typename B>
std::pair<B, A> flip_pair(const pair<A, B> &p) {
    return pair<B, A>(p.second, p.first);
}

template <typename A, typename B> multimap<B, A> flip_map(const map<A, B> &m) {
    multimap<B, A> inv;
    transform(m.begin(), m.end(), inserter(inv, inv.begin()), flip_pair<A, B>);
    return inv;
}

void write_results_to_file(string &file_name, ulong id,
                           vector<ulong> &resulting_doc_ids,
                           mutex &file_mutex) {

    lock_guard<mutex> lock(file_mutex);
    fstream file(file_name, ios::out | ios::app);
    // If file not empty add a newline before serializing output
    file << id << " " << resulting_doc_ids.size();
    for (auto e : resulting_doc_ids)
        file << " " << e;
    file << endl;

}

// (id -> score)
map<ulong, float>
cosine_similarity(string &query,
                  unordered_map<std::string, IndexVector> &inverted_index,
                  vector<float> &norms) {


    // Not counting words or using weights on the query
    // As it doesn't alter the relative ordering of the results
    set<string> query_terms;
    string word;
    stringstream ss(query);

    while (ss.good()) {
        ss >> word;
        query_terms.emplace(word);
    }

    // Value type defaults to 0, no magic required
    // (Standard defined, "default initialization")
    map<ulong, float> scores;

    for (auto t : query_terms) { // For each term t in query
        auto doc_stats = inverted_index[t];
        doc_stats.sort();
        // For each doc that contains the term
        for (IndexStat d : doc_stats.vec)
            scores[d.doc_id] += d.tf * doc_stats.idf;
    }

    // Normalize
    for (auto e : scores) {
        ulong id = e.first;
        // cout << e.second << " / " << norms[id] << endl;
        e.second = e.second / norms[id];
    }


    return scores;
}

struct query_exec_worker {

    void operator()(Queue<string, SizeLessCmp> &queue,
                    unordered_map<string, IndexVector> &inverted_index,
                    vector<float> &norms, string &file_name,
                    mutex &file_mutex) {

        string query, token;
        ulong id, k;

        while (queue.pop(query)) {
        	
            stringstream ss(query);
            // Get query id and k param for top-k
            ss >> token;
            id = stol(token);
            ss >> token;
            k = stol(token);
            // Get the query
            getline(ss, token);

            auto scores = cosine_similarity(token, inverted_index, norms);
            if (scores.empty()) continue;

            auto sorted_score_map = flip_map(scores);
            
            vector<ulong> resulting_doc_ids;
            auto it = end(sorted_score_map);
            while (it != begin(sorted_score_map) && k > 0) {
                --it;
                --k;
                resulting_doc_ids.emplace_back(it->second);
            }

            write_results_to_file(file_name, id, resulting_doc_ids, file_mutex);
        }
    }
};

void run_queries(string &query_file, string &output_file, ulong nthreads,
                 unordered_map<string, IndexVector> &inverted_index,
                 vector<float> &norms) {

    double read_time = get_clock_time();

    Queue<string, SizeLessCmp> query_queue;
    ulong query_count = read_file(query_file, query_queue);
    cout << "Query count:\t" << query_count << endl;

    // If the number of threads given is greater than the number of documents
    // Lower the number of the threads
    if (nthreads > query_count) {
        logger(string("Number of threads is greater than necessary,") +
               string(" lowering it to match number of queries."));
        nthreads = query_count;
    }

    logger("Read time:\t" + to_string(get_clock_time() - read_time));
    double query_time = get_clock_time();

    mutex file_mutex;
    if (!PARALLELIZE_QUERY_EXEC || nthreads == 1) {
        query_exec_worker t;
        t(query_queue, inverted_index, norms, output_file, file_mutex);
    } else {

        vector<thread> thread_pool;
        for (ulong i = 0; i < nthreads - 1; ++i) {
            auto t = thread(query_exec_worker(), ref(query_queue),
                            ref(inverted_index), ref(norms), ref(output_file),
                            ref(file_mutex));
            thread_pool.push_back(move(t));
        }
        // Keep the main thread busy
        query_exec_worker t;
        t(query_queue, inverted_index, norms, output_file, file_mutex);

        // this_thread::sleep_for(chrono::milliseconds(200));
        for (auto t = begin(thread_pool); t != end(thread_pool); ++t) {
            if (t->joinable())
                t->join();
            else
                cerr << "ERROR: Inactive or detached thread.\n";
        }
    }

    logger("Query time:\t" + to_string(get_clock_time() - query_time));
}

#endif // QUERY_EXECUTOR