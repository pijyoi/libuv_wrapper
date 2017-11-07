#include <vector>
#include <memory>
#include <string>
#include <algorithm>

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

    std::vector<std::unique_ptr<uvpp::Tcp>> clients;

    struct sockaddr_in saddr;
    uv_ip4_addr("0.0.0.0", 12345, &saddr);
    server.bind(saddr);
    server.listen([&](std::unique_ptr<uvpp::Tcp> conn){
        struct sockaddr_in saddr;
        conn->getpeername(saddr);
        char buffer[64];
        uv_ip4_name(&saddr, buffer, sizeof(buffer));
        printf("connection from %s:%d\n", buffer, ntohs(saddr.sin_port));

        auto pconn = conn.get();
        conn->set_callback([&clients, pconn](char *buf, int len){
            if (len < 0) {
                uvpp::print_error(len);
                clients.erase(std::remove_if(clients.begin(),
                                             clients.end(),
                                             [pconn](decltype(clients.front()) elem){
                                                return elem.get()==pconn;
                                             }),
                              clients.end());
                return;
            }

            auto str = std::string(buf, len);
            std::cout << str << std::endl;
        });
        conn->read_start();
        clients.emplace_back(std::move(conn));
    });

    uvloop.run();
}

