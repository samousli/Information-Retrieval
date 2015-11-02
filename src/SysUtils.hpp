#ifndef SYS_UTILS
#define SYS_UTILS

#include "ConcurrentQueue.hpp"
#include "Logger.hpp"

#include <iostream>
#include <cstring>

#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include <fstream>
#include <string>

typedef uint_fast64_t ulong;



inline ulong read_file(std::string &dir, Queue<std::string, SizeLessCmp> &q) {

    std::ifstream file(dir);
    std::string s;
    std::getline(file, s);
    ulong line_count = stol(s);
    for (ulong i = 0; i < line_count; ++i) {
    //while (file.good()) {
        std::getline(file, s);
        q.push(s);
    }
    file.close();
    return line_count;
}

double get_clock_time() {
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return static_cast<double>(tm.tv_sec) +
           static_cast<double>(tm.tv_usec) / 1000000.0;
}

double get_cpu_time() {
    timeval tm;
    rusage ru;
    getrusage(RUSAGE_SELF, &ru);

    // Calculates both user time and system time
    // tv_sec is the whole seconds elapsed, usec is the elapsed microseconds
    // since the last second and thus they are converted to second units
    tm = ru.ru_utime;
    double t;
    t = static_cast<double>(tm.tv_sec) +
        static_cast<double>(tm.tv_usec) / 1000000.0;
    tm = ru.ru_stime;
    t += static_cast<double>(tm.tv_sec) +
         static_cast<double>(tm.tv_usec) / 1000000.0;
    return t;
}

ulong get_file_size(const std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

#endif // SYS_UTILS