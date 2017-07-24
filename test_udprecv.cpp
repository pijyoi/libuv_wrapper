#include <cinttypes>
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

    struct sockaddr_in saddr;

    uvpp::Udp mcast(uvloop);
    uv_ip4_addr("0.0.0.0", 5353, &saddr);
    mcast.bind(saddr, UV_UDP_REUSEADDR);
    mcast.join("224.0.0.251", "0.0.0.0");
    mcast.set_callback([](char *buf, int len, const struct sockaddr *addr){
        char namebuf[32];
        uv_ip4_name((const struct sockaddr_in*)addr, namebuf, sizeof(namebuf));
        printf("got %p %d from %s\n", buf, len, namebuf);
    });
    mcast.start();

    uvpp::Udp ucast(uvloop);
    uv_ip4_addr("0.0.0.0", 0, &saddr);
    ucast.bind(saddr);
    ucast.getsockname(saddr);
    printf("listening on %d\n", ntohs(saddr.sin_port));
    int rcvbufsize = ucast.recv_buffer_size(1048576);
    printf("recv_buffer_size: %d\n", rcvbufsize);
    ucast.set_callback([](char *buf, int len, const struct sockaddr *addr){
        uint64_t *payload = (uint64_t *)buf;
        char namebuf[32];
        uv_ip4_name((const struct sockaddr_in*)addr, namebuf, sizeof(namebuf));
        printf("got packet %" PRIu64 " %d %" PRIu64 " from %s\n", payload[0], len, payload[1], namebuf);
    });
    ucast.start();

    uvloop.run();
}
