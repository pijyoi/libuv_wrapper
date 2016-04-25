#ifndef UVPP_ZSOCK_HPP
#define UVPP_ZSOCK_HPP

#include "uvpp.hpp"
#include "uv_zsock.h"

namespace uvpp
{
    uv_os_sock_t get_zsock_fd(void *zsock)
    {
        uv_os_sock_t sockfd;
        size_t optlen = sizeof(sockfd);
	    int rc = zmq_getsockopt(zsock, ZMQ_FD, &sockfd, &optlen);
	    assert(rc==0);
        return sockfd;
    }

    int get_revents(void *zsock, int events)
    {
        int revents = 0;

        int zmq_events;
        size_t optlen = sizeof(zmq_events);
        int rc = zmq_getsockopt(zsock, ZMQ_EVENTS, &zmq_events, &optlen);

        if (rc==-1) {
            // on error, make callback get called
            return events;
        }

        if (zmq_events & ZMQ_POLLOUT)
            revents |= events & UV_WRITABLE;
        if (zmq_events & ZMQ_POLLIN)
            revents |= events & UV_READABLE;

        return revents;
    }

    class ZsockWatcher
    {
    public:
        ZsockWatcher(Loop& uvloop, void *zsock)
          : m_zsock(zsock), m_events(0),
            m_prepare(uvloop), m_check(uvloop), m_idle(uvloop),
            m_poll(uvloop, get_zsock_fd(zsock))
        {
            m_prepare.set_callback([this](){
                int revents = get_revents(m_zsock, m_events);
                if (revents) {
                    // idle ensures that libuv will not block
                    m_idle.start();
                }
            });

            m_check.set_callback([this](){
                m_idle.stop();
                int revents = get_revents(m_zsock, m_events);
                if (revents) {
                    m_callback();
                }
            });

            m_idle.set_callback([](){});
            m_poll.set_callback([](int status, int events){});
        }

        void set_callback(BasicCallback cb)
        {
            m_callback = cb;
        }

        void start(int events = UV_READABLE)
        {
            m_events = events;
            m_prepare.start();
            m_check.start();
            m_poll.start(m_events ? UV_READABLE : 0);
        }

        void stop()
        {
            m_prepare.stop();
            m_check.stop();
            m_idle.stop();
            m_poll.stop();
        }

    private:
        BasicCallback m_callback;

        void *m_zsock;
        int m_events;
        Prepare m_prepare;
        Check m_check;
        Idle m_idle;
        Poll m_poll;
    };
}

#endif
