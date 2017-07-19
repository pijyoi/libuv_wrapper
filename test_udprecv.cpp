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
    struct sockaddr_in saddr;
    uv_ip4_addr("0.0.0.0", 5353, &saddr);
    udprecv.bind(saddr, UV_UDP_REUSEADDR);
    udprecv.join("224.0.0.251", "0.0.0.0");
    udprecv.set_callback([](char *buf, int len, const struct sockaddr *addr){
        char namebuf[32];
        uv_ip4_name((const struct sockaddr_in*)addr, namebuf, sizeof(namebuf));
        printf("got %p %d from %s\n", buf, len, namebuf);
    });
    udprecv.start();

    uvloop.run();
    printf("first loop break\n");
}
