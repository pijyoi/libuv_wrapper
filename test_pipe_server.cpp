#include <list>
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

    const char *pipename = argv[1];

    uvpp::Pipe server(uvloop);

    std::list<uvpp::Pipe> clients;

    server.bind(pipename);
    server.listen([&](uvpp::Pipe conn){
        auto peername = conn.getpeername();
        printf("connection from %s\n", peername.c_str());

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

