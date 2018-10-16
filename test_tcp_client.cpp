#include <string>

#include "uvpp_tcp.hpp"

int main(int argc, char *argv[])
{
    uvpp::Loop uvloop;

    uvpp::Signal sigtrap(uvloop);
    sigtrap.set_callback([&uvloop](){
        uvloop.stop();
    });
    sigtrap.start(SIGINT);
    sigtrap.unref();

    const char *node = argv[1];
    const char *service = argv[2];

    uv_getaddrinfo_t aireq;
    uv_getaddrinfo(uvloop, &aireq, NULL, node, service, NULL);

    sockaddr_storage saddr;
    memcpy(&saddr, aireq.addrinfo->ai_addr, aireq.addrinfo->ai_addrlen);

    uv_freeaddrinfo(aireq.addrinfo);

    uvpp::Tcp client(uvloop);

    client.connect((sockaddr*)&saddr, [&](int status){
        if (status < 0) {
            uvpp::print_error(status);
            return;
        }
        sockaddr_storage saddr;
        client.getpeername((sockaddr*)&saddr, sizeof(saddr));
        uv_getnameinfo_t nireq;
        uv_getnameinfo(uvloop, &nireq, NULL, (sockaddr*)&saddr, NI_NUMERICHOST | NI_NUMERICSERV);
        if (saddr.ss_family==AF_INET6) {
            printf("connected to [%s]:%s\n", nireq.host, nireq.service);
        } else {
            printf("connected to %s:%s\n", nireq.host, nireq.service);
        }

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

