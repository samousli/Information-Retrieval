#ifndef INDEX_VECTOR
#define INDEX_VECTOR

#include "IndexStat.hpp"
#include <vector>
#include <list>
#include <algorithm>

struct cmp {

	bool operator() (const IndexStat &lhs, const IndexStat &rhs) {
		return lhs.doc_id < rhs.doc_id;
	}
};

struct IndexVector {
    std::vector<IndexStat> vec;
    float idf, norm;

    IndexVector() : idf(0), norm(0) {}

    void push(const IndexStat &&stat) {
        vec.emplace_back(stat);
    }

    void sort() {
    	std::sort(vec.begin(), vec.end(), cmp());
    	//vec.sort(cmp());
    }
};


#endif
