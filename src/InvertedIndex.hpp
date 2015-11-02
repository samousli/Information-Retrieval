#ifndef INVERTED_INDEX
#define INVERTED_INDEX

#include "ConcurrentQueue.hpp"
#include "SysUtils.hpp"
#include "VectorOps.hpp"
#include "IndexStat.hpp"
#include "IndexVector.hpp"
#include "Logger.hpp"
//#include "Counters.hpp"


#include <iostream>
#include <string>
#include <sstream> // stringstream
#include <cmath>

#include <vector>
#include <map>
#include <unordered_map>
#include <utility> // make_pair

#include <thread>
#include <mutex>

#define PARTITION_THRESHOLD 1 << 23 // 4 mill ~ (4mb)
#define PARALLELIZE_INV 1
#define PARALLELIZE_IDF 1
#define PARALLELIZE_NORM 0

typedef uint_fast64_t ulong;

using namespace std; // ToDo: Check rules for namespaces on headers


// Function prototypes
ulong word_count(string &s, unordered_map<string, ulong> &wc);
ulong max_freq(unordered_map<string, ulong> &wc);

void inversion_step(unordered_map<string, IndexVector> &inverted_index,
                    Queue<string, SizeLessCmp> &file_queue, ulong doc_count,
                    uint nthreads);
void idf_step(unordered_map<string, IndexVector> &inverted_index,
              ulong doc_count, uint nthreads);
void norm_step(unordered_map<string, IndexVector> &inverted_index,
               vector<float> &norms, ulong doc_count, uint nthreads);

/**************************************************************************/

void mem_build_index(unordered_map<string, IndexVector> &inverted_index,
                     vector<float> &norms, string &dir, uint nthreads) {

    double read_time = get_clock_time();

    // Load strings into a queue
    Queue<string, SizeLessCmp> q;
    ulong doc_count = read_file(dir, q);
    cout << "Doc count:\t" << doc_count << endl;
    
    //inverted_index.reserve(inverted_index.size() 
    //	+ std::ceil(doc_count));

    // If the number of threads given is greater than the number of documents
    // Lower the number of the threads
    if (nthreads > doc_count) {
        logger(string("Number of threads is greater than necessary,") +
               string(" lowering it to match number of documents."));
        nthreads = doc_count;
    }
    logger("Read time:\t" + to_string(get_clock_time() - read_time));

    double build_time = get_clock_time();

    inversion_step(inverted_index, q, doc_count, nthreads);

    double idf_time = get_clock_time();
    logger("Inversion time:\t" + to_string(idf_time - build_time));

    idf_step(inverted_index, doc_count, nthreads);

    double norm_time = get_clock_time();
    logger("IDF time:\t" + to_string(norm_time - idf_time));

    norm_step(inverted_index, norms, doc_count, nthreads);

    logger("Norm time:\t" + to_string(get_clock_time() - norm_time));
}

/**************************************************************************/

struct inversion_worker {

    inversion_worker() {}

    void operator()(Queue<string, SizeLessCmp> &queue,
                    unordered_map<string, IndexVector> &inverted_index,
                    mutex &ii_mutex, ulong doc_count) {

        string doc, word;

        while (queue.pop(doc)) {
            // logger(to_string(doc.size()));
            // Word count and maximum frequency
            unordered_map<string, ulong> wc;
            ulong doc_id = word_count(doc, wc);
            ulong max_f = max_freq(wc);

            // Mutex locks at doc level (much faster than word level locks)
            lock_guard<mutex> lock(ii_mutex);
            for (auto t : wc)
                inverted_index[t.first].push(
                    IndexStat(doc_id, t.second, TF(t.second, max_f)));
        }
    }
};

inline void inversion_step(unordered_map<string, IndexVector> &inverted_index,
                           Queue<string, SizeLessCmp> &file_queue,
                           ulong doc_count, uint nthreads) {


    mutex ii_mutex;
    if (!PARALLELIZE_INV || nthreads == 1) {
        inversion_worker t;
        t(file_queue, inverted_index, ii_mutex, doc_count);
    } else {

        vector<thread> thread_pool;
        for (ulong i = 0; i < nthreads - 1; ++i) {
            auto t = thread(inversion_worker(), ref(file_queue),
                            ref(inverted_index), ref(ii_mutex), doc_count);
            thread_pool.push_back(move(t));
        }
        // Keep the main thread busy
        inversion_worker t;
        t(file_queue, inverted_index, ii_mutex, doc_count);

        // this_thread::sleep_for(chrono::milliseconds(200));
        for (auto t = begin(thread_pool); t != end(thread_pool); ++t) {
            if (t->joinable())
                t->join();
            else
                cerr << "ERROR: Inactive or detached thread.\n";
        }
    }
}


/**************************************************************************/
struct idf_worker {

    idf_worker() {}

    void operator()(unordered_map<string, IndexVector> &inverted_index,
                    ulong part, ulong thread_count, ulong doc_count) {

        ulong part_length = inverted_index.bucket_count() / thread_count;
        ulong lower_bound = part * part_length;
        ulong upper_bound = (part + 1) * part_length;

        // For each section
        for (ulong i = lower_bound; i < upper_bound; ++i) {
            // For each bucket in section
            for (auto t = inverted_index.begin(i); t != inverted_index.end(i);
                 ++t)
                t->second.idf = IDF(t->second, doc_count); // d, D
        }
    }
};

inline void idf_step(unordered_map<string, IndexVector> &inverted_index,
                     ulong doc_count, uint nthreads) {

    if (!PARALLELIZE_IDF || nthreads == 1) {
        idf_worker t;
        t(inverted_index, 0, nthreads, doc_count);
    } else {
        vector<thread> thread_pool;

        for (ulong i = 0; i < nthreads - 1; ++i) {
            auto t = thread(idf_worker(), ref(inverted_index), i, nthreads,
                            doc_count);
            thread_pool.push_back(move(t));
        }
        // Keep the main thread busy
        idf_worker t;
        t(inverted_index, 0, nthreads, doc_count);

        for (auto t = begin(thread_pool); t != end(thread_pool); ++t) {
            if (t->joinable())
                t->join();
            else
                cerr << "ERROR: Inactive or detached thread.\n";
        }
    }
}

/**************************************************************************/

struct norm_worker {

    norm_worker() {}

    void operator()(unordered_map<string, IndexVector> &inverted_index,
                    vector<float> &norms, mutex &norm_mutex, ulong part,
                    ulong thread_count) {

        ulong part_length = inverted_index.bucket_count() / thread_count;
        ulong lower_bound = part * part_length;
        ulong upper_bound = (part + 1) * part_length;


        for (ulong i = lower_bound; i < upper_bound; ++i)
            for (auto t = inverted_index.begin(i); t != inverted_index.end(i);
                 ++t)
                for (auto d : t->second.vec) {
                    auto n = pow(t->second.idf * d.tf, 2);
                    lock_guard<mutex> lock(norm_mutex);
                    norms[d.doc_id] += n;
                }
    }
};

inline void norm_step(unordered_map<string, IndexVector> &inverted_index,
                      vector<float> &norms, ulong doc_count, uint nthreads) {


    norms = vector<float>(doc_count, 0);
    mutex norm_mutex;

    if (!PARALLELIZE_NORM || nthreads == 1) {
        norm_worker t;
        t(inverted_index, norms, norm_mutex, 0, doc_count);
    } else {

        vector<thread> thread_pool;

        for (ulong i = 0; i < nthreads - 1; ++i) {
            auto t = thread(norm_worker(), ref(inverted_index), ref(norms),
                            ref(norm_mutex), i, doc_count);
            thread_pool.push_back(move(t));
        }
        // Keep the main thread busy
        norm_worker t;
        t(inverted_index, norms, norm_mutex, 0, doc_count);

        for (auto t = begin(thread_pool); t != end(thread_pool); ++t) {
            if (t->joinable())
                t->join();
            else
                cerr << "ERROR: Inactive or detached thread.\n";
        }
    }

    for (auto d = begin(norms); d != end(norms); ++d)
        *d = sqrt(*d);
}


/**************************************************************************/

inline ulong word_count(string &s, unordered_map<string, ulong> &wc) {
    string word;
    stringstream ss(s);

    // Get ID
    ulong doc_id;
    ss >> doc_id;
    while (ss.good()) {
        ss >> word;
        ++wc[word];
    }
    return doc_id;
}

inline ulong max_freq(unordered_map<string, ulong> &wc) {
    ulong max_f = 0;
    for (auto e : wc)
        max_f = e.second > max_f ? e.second : max_f;
    return max_f;
}


#endif // INVERTEX_INDEX
