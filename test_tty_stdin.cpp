#include "uvpp_tty_stdin.hpp"

int main()
{
    uvpp::Loop uvloop;

    uvpp::TtyStdin ttyin(uvloop);
    ttyin.set_callback([](char *buf, int len) {
        if (len==UV_EOF) {
            printf("EOF\n");
            return;
        }
        auto str = std::string(buf, len);
        std::cout << str.size() << ": " << str << std::endl;
    });
    ttyin.read_start();

    uvloop.run();
}
