#ifndef UVPP_UDP_HPP
#define UVPP_UDP_HPP

#include <string>
#include <stack>

#include "uvpp.hpp"

namespace uvpp
{
    typedef std::function<void(char *, int, const struct sockaddr*)> DataReadCallback;

    class Udp : public BaseHandle<uv_udp_t>
    {
    public:
        Udp(Loop& uvloop)
        {
            uv_udp_init_ex(uvloop, m_handle, AF_INET);
        }

        ~Udp() {
            while (!m_mempool.empty()) {
                auto ptr = m_mempool.top();
                m_mempool.pop();
                free(ptr);
            }
        }

        int bind(const struct sockaddr_in& saddr, unsigned int flags=0) {
            int rc = uv_udp_bind(m_handle, (struct sockaddr*)&saddr, flags);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int getsockname(struct sockaddr_in& saddr) {
            int namelen = sizeof(saddr);
            return uv_udp_getsockname(m_handle, (struct sockaddr*)&saddr, &namelen);
        }

        int join(const std::string& mcast_addr, const std::string& iface_addr) {
            int rc =  uv_udp_set_membership(m_handle, mcast_addr.c_str(),
                iface_addr.c_str(), UV_JOIN_GROUP);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        void set_callback(DataReadCallback cb)
        {
            m_callback = cb;
        }

        void start()
        {
            int rc = uv_udp_recv_start(m_handle,
                [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
                    auto self = static_cast<Udp*>(handle->data);
                    self->alloc_callback(suggested_size, buf);
                },
                [](uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                    const struct sockaddr *addr, unsigned flags){
                    auto self = static_cast<Udp*>(handle->data);
                    self->recv_callback(nread, buf, addr, flags);
                });
            assert(rc==0 || print_error(rc));
        }

        void stop()
        {
            int rc = uv_udp_recv_stop(m_handle);
            assert(rc==0 || print_error(rc));
        }

        int try_send(const char *buf, int len, const struct sockaddr_in& saddr) {
            uv_buf_t uvbuf = uv_buf_init(const_cast<char*>(buf), len);
            return uv_udp_try_send(m_handle, &uvbuf, 1,
                (const struct sockaddr *)&saddr);
        }

    private:
        void alloc_callback(size_t suggested_size, uv_buf_t *buf) {
            if (m_mempool.empty()) {
                buf->base = (char *)malloc(suggested_size);
                buf->len = suggested_size;
            }
            else {
                buf->base = (char *)m_mempool.top();
                buf->len = suggested_size;
                m_mempool.pop();
            }
        }

        void recv_callback(ssize_t nread, const uv_buf_t *buf,
            const struct sockaddr *addr, unsigned flags) {
            if (nread==0 && addr==NULL) {
                // EAGAIN: don't pass this to user
            }
            else if (nread < 0) {
                m_callback(buf->base, nread, addr);
            }
            else {
                m_callback(buf->base, nread, addr);
            }

            if (buf->base)
                m_mempool.push(buf->base);
        }

    private:
        DataReadCallback m_callback;
        std::stack<void*> m_mempool;
    };
}

#endif

