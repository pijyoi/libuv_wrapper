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

    uvpp::Tcp server(uvloop);

    std::list<uvpp::Tcp> clients;

    struct sockaddr_in saddr;
    uv_ip4_addr("127.0.0.1", 12345, &saddr);
    server.bind(saddr);
    server.listen([&](uvpp::Tcp conn){
        struct sockaddr_in saddr;
        conn.getpeername(saddr);
        char buffer[64];
        uv_ip4_name(&saddr, buffer, sizeof(buffer));
        printf("connection from %s:%d\n", buffer, ntohs(saddr.sin_port));

        clients.emplace_back(std::move(conn));
        auto connit = --clients.end();
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

