#include <list>
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

    uvpp::Tcp server(uvloop, AF_INET6);    // create dual-stack socket

    std::list<uvpp::Tcp> clients;

    sockaddr_in6 saddr;
    uv_ip6_addr("::", 12345, &saddr);
    server.bind((sockaddr*)&saddr);
    server.listen([&](uvpp::Tcp conn){
        sockaddr_storage saddr;
        conn.getpeername((sockaddr*)&saddr, sizeof(saddr));
        uv_getnameinfo_t nireq;
        uv_getnameinfo(uvloop, &nireq, NULL, (sockaddr*)&saddr, NI_NUMERICHOST | NI_NUMERICSERV);
        printf("connection from [%s]:%s\n", nireq.host, nireq.service);

        clients.emplace_back(std::move(conn));
        auto connit = std::prev(clients.end());
        connit->set_callback([&clients, connit](char *buf, int len){
            if (len < 0) {
                uvpp::print_error(len);
                clients.erase(connit);
                return;
            }

            auto str = std::string(buf, len);
            std::cout << str << std::endl;

            connit->write(buf, len);
        });
        connit->read_start();
    });

    uvloop.run();
}

