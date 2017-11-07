#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
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
    uv_ip4_addr("0.0.0.0", 12345, &saddr);
    mcast.bind(saddr, UV_UDP_REUSEADDR);
    mcast.getsockname(saddr);
    mcast.join("239.255.255.1", "0.0.0.0");
    // printf("listening on %d\n", ntohs(saddr.sin_port));
    int rcvbufsize = mcast.recv_buffer_size(1048576);
    printf("recv_buffer_size: %d\n", rcvbufsize);
    uint32_t sndcnt = 0;
    std::vector<int> blkcnt;
    mcast.set_callback([&](char *buf, int len, const struct sockaddr *addr){
        uint32_t *payload = (uint32_t *)buf;
        if (sndcnt!=payload[0]) {
            std::cout << sndcnt << " : [ ";
            std::ostream_iterator<int> out_it(std::cout, " ");
            std::copy(blkcnt.begin(), blkcnt.end(), out_it);
            std::cout << "]" << std::endl;
            sndcnt = payload[0];
            blkcnt.clear();
        }
        blkcnt.push_back(payload[1]);
    });
    mcast.recv_start();

    uvloop.run();
    printf("loop exit\n");
}

