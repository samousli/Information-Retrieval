#ifndef QUEUE
#define QUEUE

#include <queue>
#include <thread>
#include <mutex>
#include <vector>
//#include <condition_variable>

template <class T, class compare> class Queue {
private:
    std::priority_queue<T, std::vector<T>, compare> _queue;
    std::mutex _mutex;
    // std::condition_variable _cond;
public:
    T pop() {
        std::unique_lock<std::mutex> mlock(_mutex);

        // while (_queue.empty()) _cond.wait(mlock);

        auto item = _queue.front();
        _queue.pop();
        return item;
    }

    bool pop(T &item) {
        std::unique_lock<std::mutex> mlock(_mutex);

        // while (_queue.empty()) _cond.wait(mlock);
        if (!_queue.empty()) {
            item = _queue.top();
            _queue.pop();
            return true;
        } else
            return false;
    }

    void push(const T &item) {
        std::unique_lock<std::mutex> mlock(_mutex);
        _queue.push(item);
        mlock.unlock();
        //_cond.notify_one();
    }

    void push(T &&item) {
        std::unique_lock<std::mutex> mlock(_mutex);
        _queue.push(std::move(item));
        mlock.unlock();
        //_cond.notify_one();
    }

    ulong size() {
        std::unique_lock<std::mutex> mlock(_mutex);
        return _queue.size();
    }
};

struct SizeLessCmp {

    SizeLessCmp() {}

    bool operator()(const std::string &lhs, const std::string &rhs) {
        return lhs.size() <= rhs.size();
    }
};

#endif // QUEUE
