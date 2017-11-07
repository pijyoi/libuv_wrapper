#include <string>

#include "uvpp_tcp.hpp"

int main()
{
    uvpp::Loop uvloop;

    uvpp::Signal sigtrap(uvloop);
    sigtrap.set_callback([&uvloop](){
        uvloop.stop();
    });
    sigtrap.start(SIGINT);
    sigtrap.unref();

    uvpp::Tcp client(uvloop);

    struct sockaddr_in saddr;
    uv_ip4_addr("127.0.0.1", 12345, &saddr);
    client.connect(saddr, [&](int status){
        if (status < 0) {
            uvpp::print_error(status);
            return;
        }
        struct sockaddr_in saddr;
        client.getpeername(saddr);
        char buffer[64];
        uv_ip4_name(&saddr, buffer, sizeof(buffer));
        printf("connected to %s:%d\n", buffer, ntohs(saddr.sin_port));

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

