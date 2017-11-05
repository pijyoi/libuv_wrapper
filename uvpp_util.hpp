#ifndef UVPP_UTIL_HPP
#define UVPP_UTIL_HPP

#include <stack>
#include <memory>

namespace uvpp
{
    template <int nbytes>
    struct MemPool
    {
        std::stack<std::unique_ptr<char[]>> buffers;
        char *get() {
            char *ptr;
            if (buffers.empty()) {
                ptr = new char[nbytes];
            } else {
                ptr = buffers.top().release();
                buffers.pop();
            }
            return ptr;
        }
        void put(char *ptr) {
            buffers.push(std::unique_ptr<char[]>(ptr));
        }
        int buflen() { return nbytes; }
    };
}

#endif

