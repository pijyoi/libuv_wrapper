#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <zmq.h>

#include "uvpp_zsock.hpp"

int main()
{
    uvpp::Loop uvloop;

    uvpp::Signal usignal(uvloop);
    usignal.set_callback([&uvloop](){
      uvloop.stop();
    });
    usignal.start(SIGINT);

    void *zctx = zmq_ctx_new();
    void *zsock_recv = zmq_socket(zctx, ZMQ_PAIR);
    assert(zsock_recv!=NULL);
    int rc = zmq_bind(zsock_recv, "inproc://channel");
    assert(rc!=-1);

    void *zsock_send = zmq_socket(zctx, ZMQ_PAIR);
    assert(zsock_send!=NULL);
    rc = zmq_connect(zsock_send, "inproc://channel");
    assert(rc!=-1);

    // send 2 messages
    zmq_send(zsock_send, "MSG 1", 6, 0);
    zmq_send(zsock_send, "MSG 2", 6, 0);

    // consume 1st message
    char buffer[6];
    zmq_recv(zsock_recv, buffer, sizeof(buffer), 0);

    // now ZMQ_FD will be in non-readable state

    // correct behaviour is for the 2nd message
    // to be consumed by the callback

    uvpp::Zsock uzsock(uvloop);
    uzsock.init(zsock_recv);
    uzsock.set_callback([zsock_recv, &uvloop](){
        char buffer[6];
        zmq_recv(zsock_recv, buffer, sizeof(buffer), 0);
        buffer[sizeof(buffer)-1] = 0;
        printf("%s\n", buffer);
        uvloop.stop();
    });
    uzsock.start();

    uvloop.run();
    printf("loop exited\n");

    zmq_close(zsock_recv);
    zmq_close(zsock_send);
    zmq_ctx_destroy(zctx);
}
