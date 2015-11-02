#ifndef LOGGER
#define LOGGER

#include <iostream>
#include <mutex>

#define DEBUG 1

static std::mutex _print;

void logger(std::string &&msg) {
    if (!DEBUG)
        return;

    std::lock_guard<std::mutex> lock(_print);
    std::cout << msg << std::endl;
}

void logger(std::string &msg) {
    if (!DEBUG)
        return;

    std::lock_guard<std::mutex> lock(_print);
    std::cout << msg << std::endl;
}

#endif // LOGGER