#ifndef VECTOR_OPS
#define VECTOR_OPS

#include "IndexStat.hpp"
#include "IndexVector.hpp"

#include <iostream>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <limits> // float min

typedef uint_fast64_t ulong;

float norm(IndexVector &vec) {
    float sum = 0;
    for (auto e : vec.vec)
        sum += e.tf * e.tf;

    return sqrt(sum);
}

// Sorted input is not guaranteed anymore, working around it
float dot_product(IndexVector &lhs, IndexVector &rhs) {
    std::cerr << "Do not use dot_product!\n";
    float result = 0;
    auto it1 = lhs.vec.begin(), it2 = rhs.vec.begin();

    // Increment the smaller doc id as they are sorted..does it hold?
    while (it1 != lhs.vec.end() && it2 != rhs.vec.end()) {
        if (it1->doc_id < it2->doc_id)
            ++it1;
        else if (it1->doc_id > it2->doc_id)
            ++it2;
        else
            result += it1->tf * it2->tf;
    }

    return result;
}

float cosine_distance(IndexVector &lhs, IndexVector &rhs) {
    return (dot_product(lhs, rhs) / (norm(lhs) * norm(rhs)));
}


float TF(ulong freq, ulong max_freq) { 
    float tf = freq / (std::numeric_limits<float>::min() + max_freq);
    return tf; 
}

float IDF(IndexVector &vec, ulong doc_count) {
    float idf = log((doc_count) / (std::numeric_limits<float>::min() + vec.vec.size()) );
    return idf;
}



#endif // VECTOR_OPS
