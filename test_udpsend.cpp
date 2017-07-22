#include <stdio.h>

#include "uvpp_udp.hpp"

int main(int argc, char *argv[])
{
    struct sockaddr_in saddr;
    uv_ip4_addr(argv[1], std::stoi(argv[2]), &saddr);

    uvpp::Loop uvloop;

    uvpp::Udp ucast(uvloop);
    int sndbufsize = ucast.send_buffer_size(1048576);
    printf("send_buffer_size: %d\n", sndbufsize);
    uint64_t buf[4096] = {0};
    for (int idx=0; idx < 20; idx++) {
        buf[0] = idx;
        buf[1] = uv_hrtime();
        int rc = ucast.try_send((char*)buf, sizeof(buf), saddr);
        if (rc < 0) {
            printf("%s at idx %d\n", uv_err_name(rc), idx);
            break;
        }
    }

    uvloop.run();
}
