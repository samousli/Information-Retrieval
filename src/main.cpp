#include "InvertedIndex.hpp"
#include "QueryExecutor.hpp"
#include "ConcurrentQueue.hpp"
#include "SysUtils.hpp"
#include "VectorOps.hpp"

#include <iostream>
#include <fstream>

#include <vector>
#include <map>
#include <unordered_map>
#include <utility> // make_pair


//#define DEBUG 1

using namespace std;

typedef uint_fast64_t ulong;


int main(int argc, char *args[]) {

    ulong nthreads = thread::hardware_concurrency();
    string dir("./input.txt");
    string query_file("./queries.txt");
    string output_file("./output.txt");

    if (argc > 1) {
        int arg = atoi(args[1]);
        if (arg > 0) nthreads = static_cast<ulong>(arg);
    }
    if (argc > 2) dir = string(args[2]);
    if (argc > 3) query_file = string(args[3]);
    if (argc > 4) output_file = string(args[4]);

    cout << "Thread count:\t" << nthreads << endl;
    //  cout << "Max memory size: " << getMemorySize() / (1 << 10) << " MiB\n";

    // Build
    logger("====\tIndex Builder\t====");
    double build_begin = get_clock_time();

    unordered_map<string, IndexVector> inverted_index;
    inverted_index.max_load_factor(0.9);

    vector<float> norms;
    mem_build_index(inverted_index, norms, dir, nthreads);

    double build_end = get_clock_time();
    
    logger("Term count:\t" + to_string(inverted_index.size()));
    logger("Load factor:\t" + to_string(inverted_index.load_factor()));

    // Query
    logger("====    Query Processor\t====");
    double query_begin = get_clock_time();

    //system( (string("rm -f ") + output_file).c_str());

    run_queries(query_file, output_file, nthreads, inverted_index, norms);

    double query_end = get_clock_time();

    cout << "Inverted index built in: " << build_end - build_begin << " seconds\n";
    cout << "Queries executed in:     " << query_end - query_begin << " seconds\n";

    return 0;
}


