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
    uv_ip4_addr("127.0.0.1", 0, &saddr);
    ucast.bind(saddr);
    int rcvbufsize = ucast.recv_buffer_size(1048576);
    printf("recv_buffer_size: %d\n", rcvbufsize);
    ucast.getsockname(saddr);
    ucast.set_callback([](char *buf, int len, const struct sockaddr *addr){
        int idx = ((int *)buf)[0];
        printf("%d %d\n", idx, len);
    });
    ucast.start();
    int buf[4096];
    for (int idx=0; idx < 20; idx++) {
        buf[0] = idx;
        int rc = ucast.try_send((char*)buf, sizeof(buf), saddr);
        if (rc < 0) {
            printf("%s at idx %d\n", uv_err_name(rc), idx);
            break;
        }
    }

    uvloop.run();
    printf("first loop break\n");
}
