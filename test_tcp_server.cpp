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
    uv_ip4_addr("127.0.0.1", 12345, &saddr);
    server.bind(saddr);
    server.listen([&](uvpp::Tcp client){
        struct sockaddr_in saddr;
        client.getpeername(saddr);
        char buffer[64];
        uv_ip4_name(&saddr, buffer, sizeof(buffer));
        printf("connection from %s:%d\n", buffer, ntohs(saddr.sin_port));

        clients.emplace_back(new uvpp::Tcp(std::move(client)));
        auto pconn = clients.back().get();
        pconn->set_callback([&clients, pconn](char *buf, int len){
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

            pconn->write(buf, len);
        });
        pconn->read_start();
    });

    uvloop.run();
}

