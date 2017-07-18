#include <memory>

#include <stdio.h>

#include "uvpp_udp.hpp"

int main()
{
    uvpp::Loop uvloop;

    uvpp::Signal usignal(uvloop);
    usignal.set_callback([&uvloop](){
        uvloop.stop();
    });
    usignal.start(SIGINT);

    uvpp::UdpReceiver udprecv(uvloop);
    udprecv.bind("0.0.0.0", 5353, UV_UDP_REUSEADDR);
    udprecv.join("224.0.0.251", "0.0.0.0");
    udprecv.set_callback([](char *buf, int len){
        printf("got %p %d\n", buf, len);
    });
    udprecv.start();

    uvloop.run();
    printf("first loop break\n");
}
