#include <chrono>
#include <iostream>
#include <vector>
#include <stdio.h>

#include "uvpp_udp.hpp"

int main(int argc, char *argv[])
{
    struct sockaddr_in saddr;
    uv_ip4_addr("239.255.255.1", 12345, &saddr);

    uvpp::Loop uvloop;

    uvpp::Signal sigint(uvloop);
    sigint.set_callback([&uvloop](){
        uvloop.stop();
    });
    sigint.start(SIGINT);

    uvpp::Udp mcast(uvloop);
    int sndbufsize = mcast.send_buffer_size(1048576);
    printf("send_buffer_size: %d\n", sndbufsize);

    auto period = std::chrono::milliseconds{125};
    using clk = std::chrono::steady_clock;
    auto start_time = clk::now();

    uint32_t intcnt = 0;
    std::vector<uint32_t> buf(15360);
    uvpp::Timer timer(uvloop);
    timer.set_callback([&](){
        for (int blkcnt=0; blkcnt < 9; blkcnt++) {
            buf[0] = intcnt;
            buf[1] = blkcnt;
            mcast.send((char*)buf.data(), buf.size()*sizeof(buf[0]), saddr);
        }

        intcnt++;
        auto elapsed = clk::now() - start_time;
        auto delay = (intcnt+1)*period - elapsed;
        auto delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
        if (delay_ms < 0)
            delay_ms = 0;
        timer.start(delay_ms, 0);

        if (intcnt % 8==0) {
            std::chrono::duration<double> elapsed_sec = elapsed;
            printf("%.3f\n", elapsed_sec.count());
        }
    });
    timer.start(period.count(), 0);

    uvloop.run();
    printf("loop exit\n");

    timer.stop();
    while (mcast.is_active()) {
        fprintf(stderr, ".");
        uvloop.run(UV_RUN_ONCE);
    }
}
