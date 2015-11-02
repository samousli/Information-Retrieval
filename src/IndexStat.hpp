#ifndef INDEX_STAT
#define INDEX_STAT

#include <cstdint> 	// uint_fast64_t

typedef uint_fast64_t ulong;

struct IndexStat {
    ulong doc_id, freq;
    float tf;

    IndexStat(ulong d, ulong f, float t)
        : doc_id(d), freq(f), tf(t) {}
};


#endif //INDEX_STAT
