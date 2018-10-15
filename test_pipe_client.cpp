#include <string>

#include "uvpp_pipe.hpp"

int main(int argc, char *argv[])
{
    uvpp::Loop uvloop;

    uvpp::Signal sigtrap(uvloop);
    sigtrap.set_callback([&uvloop](){
        uvloop.stop();
    });
    sigtrap.start(SIGINT);
    sigtrap.unref();

    const char *pipename = argv[1];

    uvpp::Pipe client(uvloop);

    client.connect(pipename, [&](int status){
        if (status < 0) {
            uvpp::print_error(status);
            return;
        }
        auto peername = client.getpeername();
        printf("connected to %s\n", peername.c_str());

        std::string msg = "hello world";
        client.write(msg.data(), msg.size());
        client.set_callback([&client](char *buf, int len){
            if (len < 0) {
                uvpp::print_error(len);
                return;
            }
            auto str = std::string(buf, len);
            std::cout << str << std::endl;

            client.read_stop();
        });
        client.read_start();
    });

    uvloop.run();
}

